//
// Tracking.cpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_TRACKING_DEBUG_LEVEL)

#include "module/ModuleBase.hpp"
#include "tracking/tracking.hpp"
#include "port/Thread.hpp"
#include "utility/LogSection.hpp"
#include "utility/time.hpp"
#include "utility/cont/CircularBuffer.hpp"
#include "utility/thr/WorkQueue.hpp"
#include <embmosse/Mosse.hpp>
#include "Tracking.hpp"

namespace Trk {

Mosse::Port::Thread &mosseThreadApi()
{
	static Thread thr;

	return thr;
}

static constexpr std::size_t knMosseThreads = 2;

Tracking::Tracking() :
	ModuleBase{Mod::Module::Tracking},
	tracker{nullptr},
	state{State::CamConfStart},
	key{{&Tracking::onFrame, this}},
	spinlock{Spinlock::Done}
{
}

void Tracking::onFrame(const std::shared_ptr<Cam::Frame> &aFrame)
{
	assert(nullptr != aFrame.get());
	// TODO: push into work queue, probably will cause stack overflow
	switch (state) {
		ESP_LOGI(Trk::kDebugTag, "Profile, onFrame");
		case State::CamConfStart: {  // Initialize the camera, switch it to grayscale mode
			if (Ut::Thr::Wq::MediumPriority::checkInstance()) {
				Ut::Thr::Wq::MediumPriority::getInstance().push(
					[this]()
					{
						Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>(
							"grayscale",
							[this](const Mod::Fld::WriteResp &aWriteResp)
							{
								if (aWriteResp.isOk()) {
									ESP_LOGI(Trk::kDebugTag, "Tracking: switched camera to grayscale mode");
									state = State::TrackerInit;
								} else {
									ESP_LOGE(Trk::kDebugTag, "Tracking: failed to switch the camera to grayscale mode %s",
										aWriteResp.resultAsCstr());
									state = State::CamConfFailed;
								}
							});
					});
			}

			break;
		}
		case State::TrackerInit: {
			if (static_cast<bool>(aFrame)) {

				assert(aFrame.get()->data() != nullptr);
				if (tracker == nullptr) {
					ESP_LOGI(Trk::kDebugTag, "Tracking: initializing tracker. Frame size: %dx%d", aFrame.get()->width(),
						aFrame.get()->height());
					tracker = &Mosse::getFp16AbRawF32BufDynAlloc();
				}

				ESP_LOGI(Trk::kDebugTag, "initializing tracker");
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};
				Mosse::Tp::Roi roi{{0, 0}, {64, 64}};  // TODO Missing frame size bounds check
				ESP_LOGI(Trk::kDebugTag, "Tracking: initializing tracker w/ a new ROI");
				tracker->init(image, roi);
				state = State::TrackerRunning;
				ESP_LOGI(Trk::kDebugTag, "Tracking: initialized tracker w/ a new ROI");
			} else {
				ESP_LOGW(Trk::kDebugTag, "Tracking: nullptr frame");
			}

			break;
		}
		case State::TrackerRunning: {
			if (static_cast<bool>(aFrame)) {
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};

				while (spinlock == Spinlock::Wait) {
					ESP_LOGV(Trk::kDebugTag, "Tracking: waiting for spinlock");
					// empty
				}

				auto imageWorkingArea = tracker->imageCropWorkingArea(image);
				spinlock = Spinlock::Wait;

				// TODO: sync
				if (Ut::Thr::Wq::MediumPriority::checkInstance()) {
					// Detach from the current thread to release the buffer
					Ut::Thr::Wq::MediumPriority::getInstance().push(
						[this, imageWorkingArea]() mutable
						{
							ESP_LOGV(Trk::kDebugTag, "Tracking: updating tracker");
							tracker->update(imageWorkingArea, true);
							spinlock = Spinlock::Done;
							ESP_LOGV(Trk::kDebugTag, "Tracking: updated tracker");
						},
						Ut::Thr::Wq::TaskPrio::Tracker);
				}
			}

			break;
		}
		default:
			ESP_LOGI(Trk::kDebugTag, "Tracking: Skipping frame");
			break;
	}
}

}  // namespace Trk
