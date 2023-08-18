//
// system.hpp
//
// Created: 2023-08-18
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_SYSTEM_DEBUG_LEVEL)
#include <esp_log.h>

#include "system.hpp"

namespace Sys {

void init()
{
	esp_log_level_set(Sys::kDebugTag, (esp_log_level_t)CONFIG_SYSTEM_DEBUG_LEVEL);
	ESP_LOGD(Sys::kDebugTag, "Debug log test");
	ESP_LOGV(Sys::kDebugTag, "Verbose log test");
}

}  // namespace Sys
