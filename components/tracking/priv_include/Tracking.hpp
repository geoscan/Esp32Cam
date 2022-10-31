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
	enum class Spinlock {
		Wait,
		Done,
	};

	struct Roi {
		/// \brief Frame size before and after camera reconfiguration might change. Normalized ROI enables transforming
		/// the ROI
		struct Normalized {
			float row;
			float col;
			float nrows;
			float ncols;
		} normalized = {0.0f, 0.0f};

		void initNormalized(const Mosse::Tp::Roi &absolute);  ///< Converts absolute to normalized ROI using the currently used frame size
		Mosse::Tp::Roi absolute();  ///< Converts normalized to absolute ROI using the currently used frame size
	};
private:
	struct Key {
		Sub::Key::NewFrame newFrame;
	};
	enum class State {
		CamConfStart,  ///< Tracker will only work with u8 frames, so the camera has to be configured appropriately
		CamConfFailed,
		TrackerInit,  ///< Initialize tracker with a first ROI
		TrackerRunning
	};
public:
	Tracking();
	void onFrame(const std::shared_ptr<Cam::Frame> &);  ///< Subscription handler
protected:
	void setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb) override;
private:
	Mosse::Tracker *tracker;
	State state;
	Key key;
	Spinlock spinlock;
	Roi roi;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_TRACKING_HPP_
