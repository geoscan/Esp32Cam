//
// Storage.cpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_CAMERA_RECORDER_DEBUG_LEVEL)
#include <esp_log.h>

#include "camera_recorder/Storage.hpp"
#include "camera_recorder/camera_recorder.hpp"
#include "utility/String.hpp"
#include "utility/LogSection.hpp"
#include "sd_fat.h"
#include <dirent.h>
#include <sdkconfig.h>
#include <esp_log.h>

namespace CameraRecorder {

/// \brief Counts frames stored on SD card
/// \return `ESP_ERR_NOT_FOUND` if no storage is available
///
Storage::Storage() : Mod::ModuleBase{Mod::Module::Camera}
{
	ESP_LOGI(CameraRecorder::kDebugTag, "initializing Storage");
}

void Storage::getFieldValue(Mod::Fld::Req aRequest, Mod::Fld::OnResponseCallback aOnResponse)
{
	GS_UTILITY_LOG_SECTIONV(CameraRecorder::kDebugTag, "Storage::getFieldValue");
	switch (aRequest.field) {
		case Mod::Fld::Field::CaptureCount: {
			unsigned count = 0;

			if (ESP_OK == countFrames(count)) {
				aOnResponse(makeResponse<Mod::Module::Camera, Mod::Fld::Field::CaptureCount>(count));
			}

			break;
		}

		default:
			break;
	}
}

esp_err_t Storage::countFrames(unsigned &aCountOut)
{
	GS_UTILITY_LOG_SECTIONV(CameraRecorder::kDebugTag, "Storage::countFrames");
	esp_err_t ret = ESP_ERR_NOT_FOUND;

	auto *dp = opendir(CONFIG_SD_FAT_MOUNT_POINT);
	dirent *ep;
	aCountOut = 0;

	if (nullptr != dp) {
		while ((ep = readdir(dp))) {
			ESP_LOGV(CameraRecorder::kDebugTag, "scanning a directory entry %s", ep->d_name);

			aCountOut += Ut::Str::checkEndswith(ep->d_name, ".jpg") +
				Ut::Str::checkEndswith(ep->d_name, ".jpeg") + Ut::Str::checkEndswith(ep->d_name, ".JPG") +
				Ut::Str::checkEndswith(ep->d_name, ".JPEG") + Ut::Str::checkEndswith(ep->d_name, ".avi") +
				Ut::Str::checkEndswith(ep->d_name, ".AVI");
		}

		ret = ESP_OK;
	} else {
		ESP_LOGW(CameraRecorder::kDebugTag, "Storage::countFrames unable to open directory");
	}


	return ret;
}

}  // namespace CameraRecorder
