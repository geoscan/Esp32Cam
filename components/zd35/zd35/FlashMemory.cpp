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

Sys::Error FlashMemory::writeBlock(std::uint32_t aWriteBlockId, std::uint32_t aWriteBlockOffset,
		const std::uint8_t *aData, std::size_t aDataLength)
{
	if (espFlash == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s: %s", kLogPreamble, __func__,
			kUninitializedEspFlashErrorMessage);

		return {kUninitializedEspFlashErrorMessage};
	}

	if (aData == nullptr) {
		return {Sys::ErrorCode::Nullptr};
	}

	const auto writeResult = esp_flash_write(espFlash, static_cast<const void *>(aData),
		getFlashMemoryGeometry().convertWriteBlockOffsetIntoAddress(aWriteBlockId) + aWriteBlockOffset, aDataLength);

	if (writeResult != ESP_OK) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s::%s: could not write onto chip, error code=%d message=\"%s\"", kLogPreamble, __func__, writeResult,
			esp_err_to_name(writeResult));
		return {Sys::ErrorCode::FlashMemory, esp_err_to_name(writeResult) /* Implemented as static table under the hood, no need to control lifetime */};
	}

	return {};
}

Sys::Error FlashMemory::readBlock(std::uint32_t aReadBlockId, std::uint32_t aReadBlockOffset, std::uint8_t *aBuffer,
	std::size_t aBufferSize)
{
	if (espFlash == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s: %s", kLogPreamble, __func__,
			kUninitializedEspFlashErrorMessage);

		return {kUninitializedEspFlashErrorMessage};
	}

	const auto readResult = esp_flash_read(espFlash, aBuffer,
		getFlashMemoryGeometry().convertWriteBlockOffsetIntoAddress(aReadBlockId) + aReadBlockOffset, aBufferSize);

	if (readResult != ESP_OK) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s::%s: could not read from memory error code=%d message=\"%s\"", kLogPreamble, __func__, readResult,
			esp_err_to_name(readResult));

		return {Sys::ErrorCode::FlashMemory, esp_err_to_name(readResult)};
	}

	return {};
}

Sys::Error FlashMemory::eraseBlock(std::uint32_t aEraseBlockId)
{
	if (espFlash == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s: %s", kLogPreamble, __func__,
			kUninitializedEspFlashErrorMessage);

		return {kUninitializedEspFlashErrorMessage};
	}

	const auto eraseResult = esp_flash_erase_region(espFlash,
		getFlashMemoryGeometry().convertEraseBlockOffsetIntoAddress(aEraseBlockId),
		getFlashMemoryGeometry().eraseBlockSize);

	if (eraseResult != ESP_OK) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s: failed to erase block starting from %d error (%d) (%s)", kLogPreamble, __func__,
			aEraseBlockId, eraseResult, esp_err_to_name(eraseResult));

		return {Sys::ErrorCode::FlashMemory, esp_err_to_name(eraseResult)};
	}

	return {};
}

}  // Zd35
