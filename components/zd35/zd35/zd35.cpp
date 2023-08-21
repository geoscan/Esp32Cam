//
// zd35.hpp
//
// Created: 2023-08-21
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL)
#include <esp_log.h>

#include "zd35.hpp"

namespace Zd35 {

void init()
{
	esp_log_level_set(Zd35::debugTag(), (esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL);
	ESP_LOGD(Zd35::debugTag(), "Debug log test");
	ESP_LOGV(Zd35::debugTag(), "Verbose log test");
}

}  // namespace Zd35
