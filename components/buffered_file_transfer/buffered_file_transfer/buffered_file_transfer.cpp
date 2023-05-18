//
// buffered_file_transfer.hpp
//
// Created: 2023-05-18
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_BUFFERED_FILE_TRANSFER_DEBUG_LEVEL)
#include <esp_log.h>

#include "buffered_file_transfer.hpp"

namespace Bft {

void init()
{
	esp_log_level_set(Bft::kDebugTag, (esp_log_level_t)CONFIG_BUFFERED_FILE_TRANSFER_DEBUG_LEVEL);
	ESP_LOGD(Bft::kDebugTag, "Debug log test");
	ESP_LOGV(Bft::kDebugTag, "Verbose log test");
}

}  // namespace Bft
