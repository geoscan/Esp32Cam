//
// camera_recorder.cpp
//
// Created on: May 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_CAMERA_RECORDER_DEBUG_LEVEL)
#include <esp_log.h>

#include "camera_recorder/RecFrame.hpp"
#include "camera_recorder/RecMjpgAvi.hpp"
#include "camera_recorder/StopRecordingWifiDisconnectHandler.hpp"
#include "camera_recorder/Storage.hpp"
#include "camera_recorder/camera_recorder.hpp"

namespace CameraRecorder {

const char *kDebugTag = "[camera_recorder]";

void init()
{
#ifdef CONFIG_CAMERA_RECORDER_ENABLE
	static RecFrame recFrame{};
	static RecMjpgAvi recMjpgAvi{};
	static Storage storage{};
	static StopRecordingWifiDisconnectHandler stopRecordingWifiDisconnectHandler{&recMjpgAvi};
	(void)storage;
	esp_log_level_set(CameraRecorder::kDebugTag, (esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL);
	ESP_LOGD(CameraRecorder::kDebugTag, "Debug log test");
	ESP_LOGV(CameraRecorder::kDebugTag, "Verbose log test");
#endif
}

}  // namespace CameraRecorder
