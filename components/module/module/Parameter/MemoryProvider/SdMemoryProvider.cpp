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
	Result res = configFileEnsureExists();

	// Open the file, read its content, and close it
	if (res == Result::Ok) {
		FILE *json = fopen(kParametersFileName, "rb");
		assert(json != nullptr);  // We've already checked it exists. If it doesn't, `configFileEnsureExist` is implemented incorrectly
		const std::size_t jsonSize = Ut::Sys::Fs::fileSize(json);
		jsonBytes = std::unique_ptr<std::uint8_t[]>{new std::uint8_t[jsonSize + 1]{0}};
		const std::size_t nread = fread(jsonBytes.get(), 1, jsonSize, json);

		if (nread != jsonSize) {
			res = Result::FileIoError;
		}

		if (json != nullptr) {
			fclose(json);
		}
	}

	return res;
}

}  // namespace Par
}  // namespace Mod
