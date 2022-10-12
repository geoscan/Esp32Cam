//
// tracking.hpp
//
// Created: 2022-09-30
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_TRACKING_DEBUG_LEVEL)
#include <esp_log.h>
#include "tracking.hpp"
#include "Profile.hpp"
#include "port/OsApi.hpp"
#include <embmosse/Mosse.hpp>
#include <esp_log.h>

namespace Trk {

static void logInit()
{
	esp_log_level_set(Trk::kDebugTag, (esp_log_level_t)CONFIG_TRACKING_DEBUG_LEVEL);
	ESP_LOGD(Trk::kDebugTag, "Debug log test");
	vTaskDelay(1);
	ESP_LOGV(Trk::kDebugTag, "Verbose log test");
}

static void apiInit()
{
	static OsApi osApi;
	(void)osApi;
}

void init()
{
	apiInit();
	logInit();
#if CONFIG_TRACKING_RUN_PROFILE
	profileInit();
#endif
}

void profileInit()
{
	logInit();
	apiInit();
	static Profile profile;
	(void)profile;
}

}  // namespace Trk
