//
// COMPONENT_FRAME.hpp
//
// Created: 
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_COMPONENT_FRAME_UPPER_DEBUG_LEVEL)
#include <esp_log.h>

#include "COMPONENT_FRAME.hpp"

namespace COMPONENT_FRAME_NAMESPACE {

void init()
{
	esp_log_level_set(COMPONENT_FRAME_NAMESPACE::kDebugTag, (esp_log_level_t)CONFIG_COMPONENT_FRAME_DEBUG_LEVEL);
	ESP_LOGD(COMPONENT_FRAME_NAMESPACE::kDebugTag, "Debug log test");
	ESP_LOGV(COMPONENT_FRAME_NAMESPACE::kDebugTag, "Verbose log test");
}

}  // namespace COMPONENT_FRAME_NAMESPACE
