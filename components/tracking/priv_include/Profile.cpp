//
// Profile.cpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_TRACKING_DEBUG_LEVEL)

#include "module/ModuleBase.hpp"
#include "tracking/tracking.hpp"
#include "Profile.hpp"
#include "port/Thread.hpp"
#include "utility/LogSection.hpp"
#include "utility/time.hpp"
#include "utility/cont/CircularBuffer.hpp"
#include "utility/thr/WorkQueue.hpp"
#include <embmosse/Mosse.hpp>

namespace Trk {

Mosse::Port::Thread &mosseThreadApi()
{
	static Thread thr;

	return thr;
}

static constexpr std::size_t knMosseThreads = 2;

Profile::Profile() :
	tracker{nullptr},
	state{State::CamConfStart},
	key{{&Profile::onFrame, this}}
{
}

struct FpsCounter {
	Ut::Cont::CircularBuffer<std::chrono::microseconds, 10, true> timestamps;

	float onFrame()
	{
		timestamps.push_back(std::chrono::microseconds{Ut::bootTimeUs()});
		float ret = 0.0f;

		if (timestamps.size() > 1) {
			ret = static_cast<float>(timestamps.size() + timestamps.capacity()) / (static_cast<float>((timestamps.back()
				- timestamps.front()).count()) / 1000000.0f);
		}

		return ret;
	}
};

static FpsCounter sFpsCounter;

void Profile::onFrame(const std::shared_ptr<Cam::Frame> &aFrame)
{
	assert(nullptr != aFrame.get());
	// TODO: push into work queue, probably will cause stack overflow
	switch (state) {
		ESP_LOGI(Trk::kDebugTag, "Profile, onFrame");
		case State::CamConfStart: {  // Initialize the camera, switch it to grayscale mode
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>("grayscale",
				[this](const Mod::Fld::WriteResp &aWriteResp)
				{
					if (aWriteResp.isOk()) {
						ESP_LOGI(Trk::kDebugTag, "Profile: switched camera to grayscale mode");
						state = State::TrackerInit;
					} else {
						ESP_LOGE(Trk::kDebugTag, "Profile: failed to switch the camera to grayscale mode %s",
							aWriteResp.resultAsCstr());
						state = State::CamConfFailed;
					}
				});

			break;
		}
		case State::TrackerInit: {
			if (static_cast<bool>(aFrame)) {

				// The implementation allows using managed threads. It means that SIMD operations pertaining to assigned frame
				// chunks will be parallelized, all except the managed one which will be run in the current thread
				static constexpr float kSplit0Fraction = 0.5f;
				static constexpr std::size_t kManagedSplitId = 0;

				assert(aFrame.get()->data() != nullptr);
				if (tracker == nullptr) {
					ESP_LOGI(Trk::kDebugTag, "Profile: initializing tracker. Frame size: %dx%d", aFrame.get()->width(),
						aFrame.get()->height());
					tracker = &Mosse::getFp16AbRawF32BufDynAlloc();
				}

				ESP_LOGI(Trk::kDebugTag, "initializing tracker");
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};
				Mosse::Tp::Roi roi{{0, 0}, {64, 64}};  // TODO Missing frame size bounds check
				ESP_LOGI(Trk::kDebugTag, "Profile: initializing tracker w/ a new ROI");
				tracker->init(image, roi);
				state = State::TrackerRunning;
				ESP_LOGI(Trk::kDebugTag, "Profile: initialized tracker w/ a new ROI");
			} else {
				ESP_LOGW(Trk::kDebugTag, "Profile: nullptr frame");
			}

			break;
		}
		case State::TrackerRunning: {
			if (static_cast<bool>(aFrame)) {
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};
				auto imageWorkingArea = tracker->imageCropWorkingArea(image);

#if CONFIG_TRACKING_RUN_PROFILE_TYPE_FULL
				// TODO: sync
				if (Ut::Thr::Wq::MediumPriority::checkInstance()) {
					// Detach from the current thread to release the buffer
					Ut::Thr::Wq::MediumPriority::getInstance().push(
						[this, imageWorkingArea]() mutable
						{
							ESP_LOGV(Trk::kDebugTag, "Profile: updating tracker");
							tracker->update(imageWorkingArea, true);
							ESP_LOGV(Trk::kDebugTag, "Profile: updated tracker");
						},
						Ut::Thr::Wq::TaskPrio::Tracker);
				}

				outputProfile();
#elif CONFIG_TRACKING_RUN_PROFILE_TYPE_TRACKER
				while (true) {
					constexpr std::size_t kDelayCounterReset = 10;

					for (std::size_t i = 0; i < kDelayCounterReset; ++i) {
						ESP_LOGV(Trk::kDebugTag, "Profile: updating tracker");
						tracker->update(imageWorkingArea, true);
						ESP_LOGV(Trk::kDebugTag, "Profile: updated tracker");
						outputProfile();
					}

					vTaskDelay(1);
				}
#elif CONFIG_TRACKING_RUN_PROFILE_TYPE_CAMERA
				outputProfile();
#endif
			}

			break;
		}
		default:
			ESP_LOGI(Trk::kDebugTag, "Profile: Skipping frame");
			break;
	}
}

void Profile::outputProfile()
{
	constexpr float kPsrThreshold = 0.05f;
	ESP_LOGI(Trk::kDebugTag, "Profile: psr %.4f fps %.2f", tracker->lastPsr(), sFpsCounter.onFrame());

	if (tracker->lastPsr() < kPsrThreshold) {
		ESP_LOGE(Trk::kDebugTag, "PSR lower threshold has been exceeded");
	}
}

}  // namespace Trk
