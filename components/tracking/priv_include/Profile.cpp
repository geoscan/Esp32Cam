//
// Profile.cpp
//
// Created on: Sep 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Mosse.hpp"
#include "Profile.hpp"

namespace Trk {

Profile::Profile() : key{{&Profile::onFrame, this}}
{
}

void Profile::onFrame(const std::shared_ptr<Cam::Frame> &)
{
}

}  // namespace Trk
