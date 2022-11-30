//
// SdMemoryProvider.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MODULE_DEBUG_LEVEL)
#include <esp_log.h>
#include "sd_fat.h"
#include "utility/system/Fs.hpp"
#include "utility/LogSection.hpp"
#include "module/Parameter/ParameterDescription.hpp"
#include "module/Parameter/Variant.hpp"
#include "module/module.hpp"
#include <sdkconfig.h>
#include <cJSON.h>
#include <stdio.h>
#include <cstring>
#include <cassert>
#include "SdMemoryProvider.hpp"

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Mod::Par::SdMemoryProvider, configFileEnsureExists, 1)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Mod::Par::SdMemoryProvider, configFileRead, 1)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Mod::Par::SdMemoryProvider, load, 1)
GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Mod::Par::SdMemoryProvider, "sdcard", 1)

namespace Mod {
namespace Par {

static constexpr const char *kParametersFileName = CONFIG_SD_FAT_MOUNT_POINT "/param.jsn";

Result SdMemoryProvider::load(const ParameterDescription &parameterDescription, Variant &variant)
{
	GS_UTILITY_LOGV_METHOD_SECTION(Mod::kDebugTag, SdMemoryProvider, load);
	Buffer buffer;
	Result res = configFileEnsureExists();
	cJSON *cjson = nullptr;

	if (res == Result::Ok) {
		res = configFileRead(buffer);
	}

	if (res == Result::Ok) {
		cjson = cJSON_Parse(buffer.get());

		if (cjson == nullptr) {
			res = Result::FileFormatError;
			configFileWriteStub();
		}
	}

	// Try to get the value
	ESP_LOGV(Mod::kDebugTag, "SdMemoryProvider::load: Parsing the value");
	if (res == Result::Ok) {
		res = Result::FileFormatError;

		if (cJSON_IsObject(cjson)) {
			cJSON *cjsonEntry = cJSON_GetObjectItemCaseSensitive(cjson, parameterDescription.name);

			if (cjsonEntry != nullptr) {
				switch (parameterDescription.parameterType) {
					case ParameterType::I32:
						if (cJSON_IsNumber(cjsonEntry)) {
							const auto cjsonEntryValue = cJSON_GetNumberValue(cjsonEntry);
							variant = Variant{static_cast<std::int32_t>(cjsonEntryValue)};
							res = Result::Ok;
						}

						break;

					case ParameterType::Str:
						if (cJSON_IsString(cjsonEntry)) {
							const auto cjsonEntryValue = cJSON_GetStringValue(cjsonEntry);
							variant = Variant{std::string{cjsonEntryValue}};
							res = Result::Ok;
						}

						break;
				}

				cJSON_Delete(cjsonEntry);
			} else {
				res = Result::EntryNotFoundError;
			}
		}
	}

	return res;
}

Result SdMemoryProvider::save(const ParameterDescription &parameterDescription, const Variant &value)
{
	// Try to open the file
	Result res = configFileEnsureExists();
	Buffer buffer;
	cJSON *cjson = nullptr;

	// Read the file's content
	if (res == Result::Ok) {
		res = configFileRead(buffer);
	}

	// Replace the corresponding JSON entry
	if (res == Result::Ok) {
		// Ensure root cJSON object
		cjson = cJSON_Parse(buffer.get());

		if (cjson != nullptr && !cJSON_IsObject(cjson)) {
			cJSON_free(cjson);
			cjson = nullptr;
		}

		if (cjson == nullptr) {
			cjson = cJSON_CreateObject();
		}

		// Replace the target entry's value
		switch (parameterDescription.parameterType) {
			case ParameterType::I32: {
				const double number = static_cast<double>(value.get_unchecked<std::int32_t>());
				cJSON_DeleteItemFromObject(cjson, parameterDescription.name);
				cJSON_AddNumberToObject(cjson, parameterDescription.name, number);

				break;
			}
			case ParameterType::Str: {
				const auto &string = value.get_unchecked<std::string>();
				cJSON_DeleteItemFromObject(cjson, parameterDescription.name);
				cJSON_AddStringToObject(cjson, parameterDescription.name, string.c_str());

				break;
			}
		}

		// Dump the result into file
		char *output = cJSON_Print(cjson);
		res = configFileWrite(output);
		assert(cjson != nullptr);
		// Deallocate memory
		cJSON_free(cjson);
		free(output);
	}

	return res;
}

Result SdMemoryProvider::configFileEnsureExists()
{
	GS_UTILITY_LOGV_METHOD_SECTION(Mod::kDebugTag, SdMemoryProvider, configFileEnsureExists);
	FILE *json = nullptr;
	constexpr std::size_t knAttempts = 2;
	Result res = Result::Ok;

	// Try to open a file. If unsuccessful, reinitialize SD/FAT, and try again
	for (std::size_t i = 0; i < knAttempts && json == nullptr; ++i) {
		json = fopen(kParametersFileName, "a");

		if (json == nullptr) {
			ESP_LOGW(Mod::kDebugTag, "SdMemoryProvider: unable to open config file %s, reinitializing SD card",
				kParametersFileName);
			sdFatDeinit();
			sdFatInit();
			res = Result::SdCardError;
		}
	}

	if (json != nullptr) {
		fclose(json);
		res = Result::Ok;
	} else {
		ESP_LOGE(Mod::kDebugTag, "SdMemoryProvider: Config file %s has not been initialized", kParametersFileName);
		sdFatDeinit();
	}

	return res;
}

Result SdMemoryProvider::configFileRead(Buffer &jsonBytes)
{
	GS_UTILITY_LOGV_METHOD_SECTION(Mod::kDebugTag, SdMemoryProvider, configFileRead);
	// Make system checks
	Result res = Result::Ok;
	constexpr const std::size_t kFileSizeUpperThreshold = 1024 * 4;  // Makes sure that the config file takes a reasonable amount of RAM
	FILE *json = nullptr;
	std::size_t jsonSize = 0;

	// Try to open the file
	if (res == Result::Ok) {
		ESP_LOGV(Mod::kDebugTag, "SdMemoryProvider::configFileRead: opening config file");
		json = fopen(kParametersFileName, "rb");

		if (json == nullptr) {
			ESP_LOGW(Mod::kDebugTag, "SdMemoryProvider: unable to open config file %s", kParametersFileName);
			res = Result::FileIoError;
		}
	}

	// Check the memory threshold
	if (res == Result::Ok) {
		assert(json != nullptr);
		jsonSize = Ut::Sys::Fs::fileSize(json);
		ESP_LOGV(Mod::kDebugTag, "SdMemoryProvider::configFileRead: calculated config file size: %dB", jsonSize);

		if (jsonSize > kFileSizeUpperThreshold) {
			ESP_LOGE(Mod::kDebugTag, "SdMemoryProvider: the file size %d B exceeds the threshold %d B. Abort reading",
				jsonSize, kFileSizeUpperThreshold);
			res = Result::NotEnoughMemoryError;
		}
	}

	// Allocate sufficient storage
	if (res == Result::Ok) {
		jsonBytes = Buffer{new char[jsonSize + 1]{0}};

		if (jsonBytes.get() == nullptr) {
			ESP_LOGE(Mod::kDebugTag, "SdMemoryProvider: could not allocate %dB, abort reading", jsonSize);
			res = Result::NotEnoughMemoryError;  // Could not allocate the required amount
		}
	}

	// Read into RAM
	if (res == Result::Ok) {
		const std::size_t nread = fread(jsonBytes.get(), 1, jsonSize, json);

		if (nread != jsonSize) {
			ESP_LOGW(Mod::kDebugTag, "SdMemoryProvider: the amount read (%dB) is not equal to estimated file size (%d)",
				nread, jsonSize);
			res = Result::FileIoError;
		}
	}

	if (json != nullptr) {
		fclose(json);
	}

	return res;
}

void SdMemoryProvider::configFileWriteStub()
{
	configFileWrite("{}");
}

Result SdMemoryProvider::configFileWrite(const char *buffer)
{
	Result res = configFileEnsureExists();
	FILE *json = nullptr;

	if (res == Result::Ok) {
		json = fopen(kParametersFileName, "wb");

		if (json == nullptr) {
			ESP_LOGE(Mod::kDebugTag, "SdMemoryProvider: unable to open configuration file %s for writing",
				kParametersFileName);
			res = Result::FileIoError;
		} else {
			fwrite(buffer, 1, strlen(buffer), json);
			fclose(json);
		}
	}

	return res;
}

}  // namespace Par
}  // namespace Mod
