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

GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Trk::Tracking, "state machine", 1);
GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Trk::Tracking, "state machine", 1);
GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Trk::Tracking, "frame", 1);
GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Trk::Tracking, "frame", 0);

namespace Trk {

extern Mosse::Port::Thread &mosseThreadApi();

Tracking::Tracking() :
	ModuleBase{Mod::Module::Tracking},
	tracker{nullptr},
	state{State::Disabled},
	key{{&Tracking::onFrame, this}},
	spinlock{Spinlock::Done},
	quality{0.0f, true}
{
}

/// \brief When in tracking mode, processes the new frame searching for the new ROI center.
void Tracking::onFrame(const std::shared_ptr<Cam::Frame> &aFrame)
{
	assert(nullptr != aFrame.get());
	GS_UTILITY_LOGV_CLASS_ASPECT(Trk::kDebugTag, Tracking, "frame", "onFrame");

	switch (state) {
		case State::CamConfStart: {
			onFrameCamConfStart(aFrame);

			break;
		}
		case State::TrackerInitFirst:
			state = State::TrackerInit;

			break;
		// TODO: Handle CamConfFailed
		case State::TrackerInit: {
			onFrameTrackerInit(aFrame);

			break;
		}
		case State::TrackerRunningFirst:
			GS_UTILITY_LOGD_CLASS_ASPECT(Trk::kDebugTag, Tracking, "state machine", "state TrackerRunningFirst");
			cameraState.currentInit();
			state = State::TrackerRunning;
			// Fall through
		case State::TrackerRunning: {
			onFrameTrackerRunning(aFrame);

			break;
		}
		default:
			break;
	}
}

void Tracking::onFrameCamConfStart(const std::shared_ptr<Cam::Frame> &)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Trk::kDebugTag, Tracking, "state machine", "state CamConfStart");
	cameraState.snapshotInit();

	// Initialize the camera, switch it to grayscale mode, because this is the only format the algorithm can work with
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
							ESP_LOGE(Trk::kDebugTag,
								"Tracking: failed to switch the camera to grayscale mode %s",
								aWriteResp.resultAsCstr());
							state = State::CamConfFailed;
						}
					});
			});
	}
}

void Tracking::onFrameTrackerInit(const std::shared_ptr<Cam::Frame> &aFrame)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Trk::kDebugTag, Tracking, "state machine", "state TrackerInit");

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

		// The ROI has to fit frame size
		if (!(r.origin(0) > 0 && r.origin(1) > 0 && r.size(0) > 0 && r.size(1) > 0
				&& r.origin(0) + r.size(0) < aFrame.get()->width()
				&& r.origin(1) + r.size(1) < aFrame.get()->height())) {
			ESP_LOGW(Trk::kDebugTag, "Tracking: failed to initialize tracker, as ROI does not fit the frame"
				"left up (%d, %d)  right bottom (%d, %d)  frame size (%d, %d)", r.origin(0), r.origin(1),
				r.origin(0) + r.size(0), r.origin(1) + r.size(1),
				aFrame.get()->width(), aFrame.get()->height());
			state = State::Disabled;
			cameraState.apply();  /// Restore camera state
		} else {
			ESP_LOGI(Trk::kDebugTag, "Tracking: initializing tracker w/ a new ROI  left up (%d, %d)  "
				"right bottom (%d, %d)  frame size (%d, %d)", r.origin(0), r.origin(1), r.origin(0) + r.size(0),
				r.origin(1) + r.size(1), aFrame.get()->width(), aFrame.get()->height());
			tracker->init(image, r);
			state = State::TrackerRunningFirst;
			ESP_LOGI(Trk::kDebugTag, "Tracking: initialized tracker w/ a new ROI");
		}
	} else {
		ESP_LOGW(Trk::kDebugTag, "Tracking: nullptr frame");
	}
}

void Tracking::onFrameTrackerRunning(const std::shared_ptr<Cam::Frame> &aFrame)
{
	GS_UTILITY_LOGV_CLASS_ASPECT(Trk::kDebugTag, Tracking, "state machine", "state TrackerRunning");

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
					quality.update(tracker->lastPsr());
					Sub::Trk::MosseTrackerUpdate mosseTrackerUpdate{
						cameraState.current.frameSize.second,  // frameHeight
						cameraState.current.frameSize.first,  // frameWidth
						nextRoi.origin(1),  // roiX (col)
						nextRoi.origin(0),  // roiY (row)
						nextRoi.size(1),  // roiWidth (# of rows)
						nextRoi.size(1),  // roiHeight (# of columns)
						quality.isOk()  // stateOk
					};
					// Notify through WQ to spare stack expenses
					Ut::Thr::Wq::MediumPriority::getInstance().push(
						[mosseTrackerUpdate]()
						{
							Sub::Trk::OnMosseTrackerUpdate::notify(mosseTrackerUpdate);
						});
					ESP_LOGV(Trk::kDebugTag, "Tracking: updated tracker, psr %.3f", tracker->lastPsr());
				},
				Ut::Thr::Wq::TaskPrio::Tracker);
		}
	}
}

/// \brief Implements tracker control API: deinitialize, initialize (a.k.a set ROI)
void Tracking::setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb)
{
	switch (aReq.field) {
		case Mod::Fld::Field::Initialized: {
			const bool initialized = aReq.variant.getUnchecked<Mod::Module::Tracking, Mod::Fld::Field::Initialized>();

			if (!initialized) {  // Request to deinitialize the tracker
				bool success = true;
				auto requestResult = Mod::Fld::RequestResult::Ok;
				const char *resultMessage = nullptr;

				if (state != State::Disabled) {
					state = State::Disabled;
					success = cameraState.apply();  // Restore the camera's previous state

					if (!success) {
						resultMessage = "Failed to restore camera state";
					}
				} else {
					requestResult = Mod::Fld::RequestResult::Other;
					resultMessage = "Ignoring tracking stop request, as it is already stopped";
					ESP_LOGW(Trk::kDebugTag, "%s", resultMessage);
					success = false;
				}

				aCb(Mod::Fld::WriteResp{requestResult, resultMessage});
			}

			break;
		}
		case Mod::Fld::Field::Roi: {
			std::array<std::uint16_t, 4> rectXywh = aReq.variant.getUnchecked<
				Mod::Module::Tracking, Mod::Fld::Field::Roi>();  // (x, y, width, height)
			Mosse::Tp::Roi roiAbsoluteRchw{{rectXywh[1], rectXywh[0]}, {rectXywh[3], rectXywh[2]}};  // (row, col, nrows=height, ncols=width)
			bool success = true;
			// Make a snapshot of the camera state, if it has not been reconfigured yet
			success = cameraState.snapshotInit();

			if (!success) {
				aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Other, "Failed to get camera state"});

				break;
			}

			success = roi.normalizedInit(roiAbsoluteRchw);

			// The (re)initialization has completed. Notify the caller
			if (success) {
				aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Ok});
				quality.reset();
			} else {
				aCb(Mod::Fld::WriteResp{Mod::Fld::RequestResult::Other, "ROI error"});
				ESP_LOGW(Trk::kDebugTag, "Failed to initialize ROI");

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

/// \brief Initializes relative (`normalized`) ROI from the absolute one, taking the current frame size into account
bool Tracking::Roi::normalizedInit(const Mosse::Tp::Roi &absolute)
{
	constexpr int kUninitialized = 0;
	std::pair<int, int> frameSize{kUninitialized, kUninitialized};
	bool ret = false;
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[&frameSize, &ret](std::pair<int, int> aFrameSize) mutable {
			frameSize = aFrameSize;
			ret = true;
		});

	if (!ret) {  // Was unable to acquire frame size
		return false;
	}

	// TODO bounds check
	if (frameSize.first != kUninitialized && absolute.size(0) != 0 && absolute.size(1) != 0 && frameSize.second != 0) {
		ret = true;
		// Normalize to frame boundaries
		normalized.row = static_cast<float>(absolute.origin(0)) / static_cast<float>(frameSize.second);
		normalized.col = static_cast<float>(absolute.origin(1)) / static_cast<float>(frameSize.first);
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
	if (snapshot.initialized) {
		return true;
	}

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
		[this, &nfields](const char *aFrameFormat)
		{
			snapshot.pixformat = aFrameFormat;
			nfields += 1;
		});

	const bool success = (nfields == knFieldsExpected);

	if (success) {
		snapshot.initialized = true;
		ESP_LOGI(Trk::kDebugTag, "Caching camera configuration: success");
	} else {
		ESP_LOGW(Trk::kDebugTag, "Caching camera configuration: failure");
	}

	return success;
}

/// \brief Captures the relevant parts of the current camera state
void Tracking::CameraState::currentInit()
{
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[this](const std::pair<int, int> &aFrameSize)
		{
			current.frameSize = aFrameSize;
		});
}

/// \brief Restores camera state using the module API
bool Tracking::CameraState::apply()
{
	bool success = true;
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameFormat>(snapshot.pixformat,
		[this, &success](Mod::Fld::WriteResp aResp)
		{
			success = success && aResp.isOk();
		});
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(snapshot.frameSize,
		[this, &success](Mod::Fld::WriteResp aResp)
		{
			success = success && aResp.isOk();
		});

	if (success) {
		ESP_LOGI(Trk::kDebugTag, "Restoring camera configuration: success");
	} else {
		ESP_LOGW(Trk::kDebugTag, "Restoring camera configuration: failure");
	}

	return success;
}

void Tracking::Quality::update(float aPsr)
{
	// TODO. W/ current optimization settings, the PSR differs drastically from that of non-optimized code. Hence the need for clamping
	psr = Ut::Al::clamp(aPsr, 0.0f, std::numeric_limits<float>::infinity());

	if (psr < lowerThreshold()) {
		if (ok) {
			ESP_LOGW(Trk::kDebugTag, "Tracking: threshold PSR %.3f, current PSR %.3f", lowerThreshold(), psr);
		}

		ok = false;
	}
}

void Tracking::Quality::reset()
{
	psr = 0.0f;
	ok = true;
}

}  // namespace Trk
