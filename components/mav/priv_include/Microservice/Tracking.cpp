//
// Tracking.cpp
//
// Created on: Nov 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Tracking.hpp"

namespace Mav {
namespace Mic {

Tracking::Tracking() : key{{&Tracking::onMosseTrackerUpdate, this, false}}
{
}

}  // namespace Mic
}  // namespace Mav
