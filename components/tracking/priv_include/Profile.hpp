//
// Profile.hpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(TRACKING_PRIV_INCLUDE_PROFILE_HPP_)
#define TRACKING_PRIV_INCLUDE_PROFILE_HPP_

#include "sub/Subscription.hpp"

namespace Mosse {

class Tracker;

}  // namespace Mosse

namespace Trk {

class Profile {
private:
	struct Key {  ///< Profile waits for camera frames routed through a subscription mechanism
		Sub::Key::NewFrame newFrame;
	};
	enum class State {
		CamConfStart,  ///< Tracker will only work with camera switched to grayscale mode
		CamConfFailed,
		TrackerInit,  ///< Initialize tracker with a first ROI
		TrackerRunning
	};
public:
	Profile();
	void onFrame(const std::shared_ptr<Cam::Frame> &);  ///< Subscription handler
private:
	Mosse::Tracker *tracker;
	State state;
	Key key;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_PROFILE_HPP_
