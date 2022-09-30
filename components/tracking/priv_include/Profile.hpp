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
	struct Key {
		Sub::Key::NewFrame newFrame;
	};
public:
	void onFrame(const std::shared_ptr<Cam::Frame> &);
private:
	Key key;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_PROFILE_HPP_
