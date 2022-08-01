//
// mav.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)


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
	ESP_LOGD(Mav::kDebugTag, "Debug log test");
	ESP_LOGV(Mav::kDebugTag, "Verbose log test");
}

}  // namespace Mav
