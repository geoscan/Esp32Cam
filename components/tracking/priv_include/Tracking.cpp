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
	state{State::Disabled},
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
				Mosse::Tp::Roi r = this->roi.absolute();
				ESP_LOGI(Trk::kDebugTag, "Tracking: initializing tracker w/ a new ROI");
				tracker->init(image, r);
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

void Tracking::setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb)
{
	switch (aReq.field) {
		case Mod::Fld::Field::Initialized: {
			const bool initialized = aReq.variant.getUnchecked<Mod::Module::Tracking, Mod::Fld::Field::Initialized>();

			if (!initialized) {
				state = State::Disabled;
				// TODO: restore camera frame size
				// TODO: restore camera pixformat
				// TODO: deinitialize the tracker
				// TODO: response
			}

			break;
		}
		case Mod::Fld::Field::Roi: {
			std::array<std::uint16_t, 4> rectXywh = aReq.variant.getUnchecked<
				Mod::Module::Tracking, Mod::Fld::Field::Roi>();  // (x, y, width, height)
			Mosse::Tp::Roi roiAbsoluteRchw{{rectXywh[1], rectXywh[0]}, {rectXywh[3], rectXywh[2]}};  // (row, col, nrows=height, ncols=width)
			bool success = true;

			// Make a snapshot of the camera state, if it has not been reconfigured yet (i.e. the tracker is called for the first time)
			if (!stateIsCameraConfigured()) {
				success = cameraState.update();

				if (!success) {
					aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Other, "Failed to get camera state"});

					break;
				}
			}

			success = roi.initNormalized(roiAbsoluteRchw);

			if (success) {
				aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Ok});
			} else {
				aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Other, "ROI error"});

				break;
			}

			if (stateIsCameraConfigured()) {
				state = State::TrackerInit;
			} else {
				state = State::CamConfStart;
			}

			break;
		}
		default:
			break;
	}
}

bool Tracking::Roi::initNormalized(const Mosse::Tp::Roi &absolute)
{
	constexpr int kUninitialized = 0;
	std::pair<int, int> frameSize{kUninitialized, kUninitialized};
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[&frameSize](std::pair<int, int> aFrameSize) mutable { frameSize = aFrameSize; });
	bool ret = false;

	// TODO bounds check
	if (frameSize.first != kUninitialized && absolute.size(0) != 0 && absolute.size(1) != 0 && frameSize.second != 0) {
		ret = true;
		normalized.row = static_cast<float>(absolute.origin(0)) / static_cast<float>(absolute.size(0));
		normalized.col = static_cast<float>(absolute.origin(1)) / static_cast<float>(absolute.size(1));
		normalized.nrows = static_cast<float>(absolute.size(0)) / static_cast<float>(frameSize.second);
		normalized.ncols = static_cast<float>(absolute.size(1)) / static_cast<float>(frameSize.first);
	}

	return ret;
}

Mosse::Tp::Roi Tracking::Roi::absolute()
{
	constexpr int kUninitialized = 0;
	std::pair<int, int> frameSize{kUninitialized, kUninitialized};
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[&frameSize](std::pair<int, int> aFrameSize) mutable { frameSize = aFrameSize; });
	Mosse::Tp::Roi roi{{0, 0}, {0, 0}};

	if (frameSize.first != kUninitialized) {
		roi.origin(0) = static_cast<Eigen::Index>(frameSize.second * normalized.row);
		roi.origin(1) = static_cast<Eigen::Index>(frameSize.first * normalized.col);
		roi.size(0) = static_cast<Eigen::Index>(normalized.nrows * frameSize.second);
		roi.size(1) = static_cast<Eigen::Index>(normalized.ncols * frameSize.first);
	}

	return roi;
}

bool Tracking::CameraState::update()
{
	constexpr int knFieldsExpected = 3;
	int nfields = 0;

	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::Initialized>(
		[&nfields](bool aInitialized) {
			nfields += static_cast<int>(aInitialized);
		});

	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[this, &nfields](const std::pair<int, int> &aFrameSize)
		{
			frameSize = aFrameSize;
			nfields += 1;
		});

	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>(
		[this, &nfields](const std::tuple<std::uint8_t, const char *> &aFrameFormat)
		{
			pixformat = std::get<1>(aFrameFormat);
			nfields += 1;
		});

	return nfields == knFieldsExpected;
}

}  // namespace Trk
