//
// SdMemoryProvider.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sd_fat.h"
#include "utility/system/Fs.hpp"
#include <sdkconfig.h>
#include <stdio.h>
#include <cstring>
#include "SdMemoryProvider.hpp"
#include <cassert>

namespace Mod {
namespace Par {

static constexpr const char *kParametersFileName = CONFIG_SD_FAT_MOUNT_POINT "/param.json";

Result SdMemoryProvider::load(const ParameterDescription &parameterDescription, Variant &variant)
{
	return Result::Ok;
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

Result SdMemoryProvider::configFileRead(std::unique_ptr<uint8_t[]> &jsonBytes)
{
	// Make system checks:
	Result res = Result::Ok;
	constexpr const std::size_t kFileSizeUpperThreshold = 1024 * 4;  // Makes sure that the config file takes a reasonable amount of RAM
	FILE *json = nullptr;
	std::size_t jsonSize = 0;

	// Try to open the file
	if (res == Result::Ok) {
		FILE *json = fopen(kParametersFileName, "rb");
		assert(json != nullptr);  // We've already checked it exists. If it doesn't, `configFileEnsureExist` is implemented incorrectly.

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
		jsonBytes = std::unique_ptr<std::uint8_t[]>{new std::uint8_t[jsonSize + 1]{0}};

		if (jsonBytes.get() == nullptr) {
			res = Result::NotEnoughMemoryError;  // Could not allocate the required amount
		}
	}

	// Read into RAM
	if (res == Result::Ok) {
		const std::size_t nread = fread(jsonBytes.get(), 1, jsonSize);

		if (nread != jsonSize) {
			res = Result::FileIoError;
		}
	}

	if (json != nullptr) {
		fclose(json);
	}

	return res;
}

}  // namespace Par
}  // namespace Mod
