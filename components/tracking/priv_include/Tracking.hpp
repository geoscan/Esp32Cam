//
// Tracking.hpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(TRACKING_PRIV_INCLUDE_TRACKING_HPP_)
#define TRACKING_PRIV_INCLUDE_TRACKING_HPP_

#include "sub/Subscription.hpp"
#include "module/ModuleBase.hpp"

namespace Mosse {

class Tracker;

namespace Tp {

class Roi;

}  // namespace Tp
}  // namespace Mosse

namespace Trk {

class Tracking : Mod::ModuleBase {
private:
	/// \brief Binary spinlock variable
	///
	/// \details It's been established experimentally that frame capturing and frame processing take roughly the same
	/// time. So a spinlock can be used without risking priority inversion.
	enum class Spinlock {
		Wait,
		Done,
	};

	/// \brief
	struct Roi {
		/// \brief Intermediate inter-state scaled representation
		struct Normalized {
			float row;
			float col;
			float nrows;
			float ncols;
		} normalized = {0.0f, 0.0f, 0.0f, 0.0f};

		bool normalizedInit(const Mosse::Tp::Roi &absolute);  ///< Converts absolute to normalized ROI using the currently used frame size
		Mosse::Tp::Roi asAbsolute();  ///< Converts normalized to absolute ROI using the currently used frame size
	};

	/// \brief Subscription keys
	struct Key {
		Sub::Key::NewFrame newFrame;
	};

	/// \brief States of the tracking process
	enum class State {
		Disabled,
		CamConfStart,  ///< Tracker will only work with u8 frames, so the camera has to be configured appropriately
		CamConfFailed,
		TrackerInit,  ///< Initialize tracker with a first ROI
		TrackerRunningFirst,  ///< Certain set-up routines have to be performed before tracking can be started. Hence the use of an additional state
		TrackerRunning
	};

	/// \brief Encapsulation of the camera state. Cached state enables state restoring after tracking is done
	struct CameraState {
		struct {
			const char *pixformat;
			std::pair<int, int> frameSize;
		} snapshot;
		struct {
			std::pair<int, int> frameSize;
		} current;

		bool snapshotInit();
		void currentInit();
		bool apply();  ///< Restores camera state
	};
public:
	Tracking();
	void onFrame(const std::shared_ptr<Cam::Frame> &);  ///< Subscription handler
protected:
	void setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb) override;
private:
	inline bool stateIsCameraConfigured() const
	{
		return state > State::CamConfFailed;
	}
private:
	Mosse::Tracker *tracker;
	State state;
	Key key;
	Spinlock spinlock;
	Roi roi;
	CameraState cameraState;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_TRACKING_HPP_
