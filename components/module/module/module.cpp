//
// module.hpp
//
// Created: 2022-08-10
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MODULE_DEBUG_LEVEL)
#include <esp_log.h>

#include "module.hpp"

namespace Mod {

void init()
{
	esp_log_level_set(Mod::kDebugTag, (esp_log_level_t)CONFIG_MODULE_DEBUG_LEVEL);
	ESP_LOGD(Mod::kDebugTag, "Debug log test");
	ESP_LOGV(Mod::kDebugTag, "Verbose log test");
}

}  // namespace Mod
