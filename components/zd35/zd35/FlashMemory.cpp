//
// FlashMemory.cpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "system/os/Logger.hpp"
#include "zd35/zd35.hpp"
#include "zd35/zd35_defs.hpp"
#include <esp_flash.h>

#include "FlashMemory.hpp"

namespace Zd35 {

static constexpr const char *kUninitializedEspFlashErrorMessage = "Uninitialized `esp_flash_t` instance";
static constexpr const char *kUnalignedDataErrorMessage = "Buffer length is not a multiple of the chip's page size";
static constexpr const char *kLogPreamble = "Zd35::FlashMemory";
static constexpr Sys::ErrorCode kZd35FlashMemoryCodeBase = Sys::ErrorCode::FlashMemory;

/// \brief Will return a default-constructed instance, if `aEspFlash ==
// nullptr`
static Sys::FlashMemoryGeometry makeFlashMemoryGeometryFromEspFlash(esp_flash_t *aEspFlash);

static inline Sys::FlashMemoryGeometry makeFlashMemoryGeometryFromEspFlash(esp_flash_t *aEspFlash)
{
	if (aEspFlash == nullptr) {
		return {};
	}

	if (aEspFlash->chip_id == Zd35x2ChipId) {
		return Sys::FlashMemoryGeometry{
			.writeBlockSize = Zd35x2PageSize,
			.eraseBlockSize = Zd35x2BlockSize,
			.nEraseBlocks = Zd35x2BlocksNumber,
		};
	}

	return {};
}

FlashMemory::FlashMemory(esp_flash_t *aEspFlash):
	Sys::FlashMemory{makeFlashMemoryGeometryFromEspFlash(aEspFlash)},
	espFlash{aEspFlash}
{
}

Sys::Error FlashMemory::writeBlocks(uint32_t aWriteBlockOffset, const uint8_t *aData, std::size_t aDataLength)
{
	if (espFlash == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s: %s", kLogPreamble, __func__,
			kUninitializedEspFlashErrorMessage);

		return {kUninitializedEspFlashErrorMessage};
	}

	if (!getFlashMemoryGeometry().checkWriteLengthIsMultiple(aDataLength)) {
		return {kUnalignedDataErrorMessage};
	}

	const auto writeResult = esp_flash_write(espFlash, static_cast<const void *>(aData),
		getFlashMemoryGeometry().convertWriteBlockOffsetIntoAddress(aWriteBlockOffset), aDataLength);

	if (writeResult != ESP_OK) {
		return {Sys::ErrorCode::Fail, esp_err_to_name(writeResult) /* Implemented as static table under the hood, no need to control lifetime */};
	}

	return {};
}

Sys::Error FlashMemory::readBlocks(std::uint32_t aReadBlockOffset, std::uint32_t anReadBlocks, std::uint8_t *aBuffer,
	std::size_t aBufferSize)
{
	if (espFlash == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s: %s", kLogPreamble, __func__,
			kUninitializedEspFlashErrorMessage);

		return {kUninitializedEspFlashErrorMessage};
	}

	// TODO

	return {};
}

Sys::Error FlashMemory::eraseBlocks(std::uint32_t aEraseBlockOffset, std::uint32_t anBlocks)
{
	if (espFlash == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s: %s", kLogPreamble, __func__,
			kUninitializedEspFlashErrorMessage);

		return {kUninitializedEspFlashErrorMessage};
	}

	// TODO

	return {};
}

}  // Zd35
