//
// camera_recorder.cpp
//
// Created on: May 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_log.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_CAMERA_RECORDER_DEBUG_LEVEL)
#include "camera_recorder/camera_recorder.hpp"
#include "camera_recorder/Storage.hpp"

namespace CameraRecorder {

const char *kDebugTag = "[camera_recorder]";

void init()
{
#ifdef CONFIG_CAMERA_RECORDER_ENABLE
	static Storage storage{};
	(void)storage;
	esp_log_level_set(CameraRecorder::kDebugTag, (esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL);
	ESP_LOGD(CameraRecorder::kDebugTag, "Debug log test");
	ESP_LOGV(CameraRecorder::kDebugTag, "Verbose log test");
#endif
}

}  // namespace CameraRecorder
