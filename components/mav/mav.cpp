//
// mav.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "mav/mav.hpp"
#include "Mavlink.hpp"
#include "Marshalling.hpp"
#include "Unmarshalling.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Dispatcher.hpp"
#include <sdkconfig.h>

namespace Mav {

void init()
{
	static Dispatcher dispatcher;
	(void)dispatcher;
	esp_log_level_set(Mav::kDebugTag, (esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL);
}

}  // namespace Mav
