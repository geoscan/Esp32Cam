//
// SdMemoryProvider.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sd_fat.h"
#include "utility/system/Fs.hpp"
#include "module/Parameter/ParameterDescription.hpp"
#include "module/Parameter/Variant.hpp"
#include <sdkconfig.h>
#include <cJSON.h>
#include <stdio.h>
#include <cstring>
#include <cassert>
#include "SdMemoryProvider.hpp"

namespace Mod {
namespace Par {

static constexpr const char *kParametersFileName = CONFIG_SD_FAT_MOUNT_POINT "/param.json";

Result SdMemoryProvider::load(const ParameterDescription &parameterDescription, Variant &variant)
{
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
	FILE *json = nullptr;
	constexpr std::size_t knAttempts = 2;
	Result res = Result::Ok;

	// Try to open a file. If unsuccessful, reinitialize SD/FAT, and try again
	for (std::size_t i = 0; i < knAttempts && json == nullptr; ++i) {
		json = fopen(kParametersFileName, "a");

		if (json == nullptr) {
			sdFatDeinit();
			sdFatInit();
			res = Result::SdCardError;

			continue;
		}
	}

	if (json != nullptr) {
		fclose(json);
		res = Result::Ok;
	}

	return res;
}

Result SdMemoryProvider::configFileRead(Buffer &jsonBytes)
{
	// Make system checks:
	Result res = Result::Ok;
	constexpr const std::size_t kFileSizeUpperThreshold = 1024 * 4;  // Makes sure that the config file takes a reasonable amount of RAM
	FILE *json = nullptr;
	std::size_t jsonSize = 0;

	// Try to open the file
	if (res == Result::Ok) {
		FILE *json = fopen(kParametersFileName, "rb");

		if (json == nullptr) {
			res = Result::FileIoError;
		}
	}

	// Check the memory threshold
	if (res == Result::Ok) {
		jsonSize = Ut::Sys::Fs::fileSize(json);

		if (jsonSize > kFileSizeUpperThreshold) {
			res = Result::NotEnoughMemoryError;
		}
	}

	// Allocate sufficient storage
	if (res == Result::Ok) {
		jsonBytes = Buffer{new char[jsonSize + 1]{0}};

		if (jsonBytes.get() == nullptr) {
			res = Result::NotEnoughMemoryError;  // Could not allocate the required amount
		}
	}

	// Read into RAM
	if (res == Result::Ok) {
		const std::size_t nread = fread(jsonBytes.get(), 1, jsonSize, json);

		if (nread != jsonSize) {
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
