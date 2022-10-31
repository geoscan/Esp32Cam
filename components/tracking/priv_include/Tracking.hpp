//
// Tracking.hpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(TRACKING_PRIV_INCLUDE_TRACKING_HPP_)
#define TRACKING_PRIV_INCLUDE_TRACKING_HPP_

#include "sub/Subscription.hpp"

namespace Mosse {

class Tracker;

}  // namespace Mosse

namespace Trk {

class Tracking {
private:
	enum class Spinlock {
		Wait,
		Done,
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
private:
	Mosse::Tracker *tracker;
	State state;
	Key key;
	Spinlock spinlock;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_TRACKING_HPP_
