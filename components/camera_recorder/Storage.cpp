//
// Storage.cpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/Storage.hpp"
#include "camera_recorder/camera_recorder.hpp"
#include "utility/String.hpp"
#include "sd_fat.h"
#include <dirent.h>
#include <sdkconfig.h>

namespace CameraRecorder {

static Sub::Sys::Fld::ModuleGetField keyModuleGetField{Storage::moduleGetField};

/// \brief Counts frames stored on SD card
/// \return `ESP_ERR_NOT_FOUND` if no storage is available
///
Storage::Storage() : Sub::Sys::ModuleBase{Sub::Sys::ModuleType::Camera}
{
}

typename Sub::Sys::Fld::ModuleGetFieldMult::Ret Storage::getFieldValue(
	typename Sub::Sys::Fld::ModuleGetFieldMult::Arg<0> aRequest,
	typename Sub::Sys::Fld::ModuleGetFieldMult::Arg<1> aOnResponse)
{
	switch (aRequest.field) {
		case Sub::Sys::Fld::FieldType::CaptureCount: {
			unsigned count = 0;

			if (ESP_OK == countFrames(count)) {
				aOnResponse(makeResponse<Sub::Sys::ModuleType::Camera, Sub::Sys::Fld::FieldType::CaptureCount>(count));
			}

			break;
		}

		default:
			break;
	}
}

esp_err_t Storage::countFrames(unsigned &aCountOut)
{
	esp_err_t ret = ESP_ERR_NOT_FOUND;

	if (sdFatInit()) {
		auto *dp = opendir(CONFIG_SD_FAT_MOUNT_POINT);
		dirent *ep;
		aCountOut = 0;

		if (nullptr != dp) {
			while ((ep = readdir(dp))) {
				aCountOut += Utility::Str::checkEndswith(ep->d_name, ".jpg") +
					Utility::Str::checkEndswith(ep->d_name, ".jpeg") + Utility::Str::checkEndswith(ep->d_name, ".JPG") +
					Utility::Str::checkEndswith(ep->d_name, ".JPEG");
			}
		} else {
			ESP_LOGW(CameraRecorder::kDebugTag, "Storage::countFrames unable to open directory");
		}

		ret = ESP_OK;
	}

	return ret;
}

Sub::Sys::Fld::ModuleGetField::Ret Storage::moduleGetField(typename Sub::Sys::Fld::ModuleGetField::Arg<0> arg)
{
	Sub::Sys::Fld::ModuleGetField::Ret ret{Sub::Sys::None(), Sub::Sys::Module::Camera};

	if (arg.shouldRespond(Sub::Sys::Module::Camera)) {
		switch (arg.field) {
			case Sub::Sys::Fld::Field::CaptureCount: {
				unsigned count = 0;

				if (ESP_OK == countFrames(count)) {
					ret.variant = count;
				}

				break;
			}
			default:
				break;
		}
	}

	return ret;
}

}  // namespace CameraRecorder
