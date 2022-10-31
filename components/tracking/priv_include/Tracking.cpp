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
#include "sub/Tracking.hpp"
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

/// \brief When in tracking mode, processes the new frame searching for the new ROI center.
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
				Mosse::Tp::Roi r = this->roi.asAbsolute();
				ESP_LOGI(Trk::kDebugTag, "Tracking: initializing tracker w/ a new ROI");
				tracker->init(image, r);
				state = State::TrackerRunningFirst;
				ESP_LOGI(Trk::kDebugTag, "Tracking: initialized tracker w/ a new ROI");
			} else {
				ESP_LOGW(Trk::kDebugTag, "Tracking: nullptr frame");
			}

			break;
		}
		case State::TrackerRunningFirst:  // Falls through
			cameraState.currentInit();
			state = State::TrackerRunning;
		case State::TrackerRunning: {
			if (static_cast<bool>(aFrame)) {
				Mosse::Tp::Image image{static_cast<std::uint8_t *>(aFrame.get()->data()), aFrame.get()->height(),
					aFrame.get()->width()};

				while (spinlock == Spinlock::Wait) {
					ESP_LOGV(Trk::kDebugTag, "Tracking: waiting for spinlock");
				}

				auto imageWorkingArea = tracker->imageCropWorkingArea(image);
				spinlock = Spinlock::Wait;

				if (Ut::Thr::Wq::MediumPriority::checkInstance()) {
					// Detach from the current thread to release the buffer (the frame buffer is synchronized w/ a mutex)
					Ut::Thr::Wq::MediumPriority::getInstance().push(
						[this, imageWorkingArea]() mutable
						{
							ESP_LOGV(Trk::kDebugTag, "Tracking: updating tracker");
							tracker->update(imageWorkingArea, true);
							spinlock = Spinlock::Done;
							auto nextRoi = tracker->roi();
							Sub::Trk::MosseTrackerUpdate mosseTrackerUpdate{
								cameraState.current.frameSize.second,  // frameHeight
								cameraState.current.frameSize.first,  // frameWidth
								nextRoi.origin(1),  // roiX (col)
								nextRoi.origin(0),  // roiY (row)
								nextRoi.size(1),  // roiWidth (# of rows)
								nextRoi.size(1),  // roiHeight (# of columns)
								tracker->lastPsr()  // psr
							};
							// Notify through WQ to spare stack expenses
							Ut::Thr::Wq::MediumPriority::getInstance().push(
								[mosseTrackerUpdate]()
								{
									Sub::Trk::OnMosseTrackerUpdate::notify(mosseTrackerUpdate);
								});
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

/// \brief Implements tracker control API: deinitialize, initialize (a.k.a set ROI)
void Tracking::setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb)
{
	switch (aReq.field) {
		case Mod::Fld::Field::Initialized: {
			const bool initialized = aReq.variant.getUnchecked<Mod::Module::Tracking, Mod::Fld::Field::Initialized>();

			if (!initialized) {  // Request to deinitialize the tracker
				state = State::Disabled;
				bool success = cameraState.apply();  // Restore the camera's previous state

				if (!success) {
					aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Other, "Failed to restore camera state"});
				} else {
					aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Ok});
				}
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
				success = cameraState.snapshotInit();

				if (!success) {
					aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Other, "Failed to get camera state"});

					break;
				}
			}

			success = roi.normalizedInit(roiAbsoluteRchw);

			// The (re)initialization has completed. Notify the caller
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

/// \brief Initializes relative (`normalized`) ROI from the absolute one, using the current frame size into account
bool Tracking::Roi::normalizedInit(const Mosse::Tp::Roi &absolute)
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

/// \brief Converts relative (`normalized`) ROI to the absolute one, taking the current frame size into account.
///
/// \brief This kind of scaling is required, because the camera's frame size may (and will) change during its
/// reconfiguration for the needs of tracking.
Mosse::Tp::Roi Tracking::Roi::asAbsolute()
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

/// \brief Saves fields which will be used for restoring camera state, when the tracker is being deinitialized
bool Tracking::CameraState::snapshotInit()
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
			snapshot.frameSize = aFrameSize;
			nfields += 1;
		});

	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>(
		[this, &nfields](const std::tuple<std::uint8_t, const char *> &aFrameFormat)
		{
			snapshot.pixformat = std::get<1>(aFrameFormat);
			nfields += 1;
		});

	return nfields == knFieldsExpected;
}

/// \brief Restores camera state using the module API
bool Tracking::CameraState::apply()
{
	bool success = true;
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(snapshot.frameSize,
		[this, &success](Mod::Fld::WriteResp aResp)
		{
			success = success && aResp.isOk();
		});
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>(snapshot.pixformat,
		[this, &success](Mod::Fld::WriteResp aResp)
		{
			success = success && aResp.isOk();
		});

	return success;
}

}  // namespace Trk
