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
#include "Tracking.hpp"
#include "port/OsApi.hpp"
#include <embmosse/Mosse.hpp>
#include <esp_log.h>

namespace Trk {

static void trackingInit();

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
#ifdef CONFIG_TRACKING_INIT
	apiInit();
	logInit();
#if CONFIG_TRACKING_RUN_PROFILE
	profileInit();
#else
	trackingInit();
#endif
#endif
}

static void trackingInit()
{
#ifdef CONFIG_TRACKING_INIT
	static Trk::Tracking tracking;
	(void)tracking;
#endif
}

void profileInit()
{
#ifdef CONFIG_TRACKING_INIT
	static Profile profile;
	(void)profile;
#endif
}

}  // namespace Trk
