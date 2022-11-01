//
// Tracking.cpp
//
// Created on: Nov 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Tracking.hpp"

namespace Mav {
namespace Mic {

Tracking::Tracking() : HrTimer{ESP_TIMER_TASK, "MavTracking", true}, key{{&Tracking::onMosseTrackerUpdate, this}}
{
}

}  // namespace Mic
}  // namespace Mav
