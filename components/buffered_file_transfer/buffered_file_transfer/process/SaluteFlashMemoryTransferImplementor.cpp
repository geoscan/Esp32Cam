//
// SaluteFlashMemoryTransferImplementor.cpp
//
// Created on: Sep 27, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "SaluteFlashMemoryTransferImplementor.hpp"
#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include "utility/cont/EndiannessAwareRepresentation.hpp"
#include <algorithm>

namespace Bft {

static constexpr const char *kLogPreamble = "SaluteFlashMemoryTransferImplementor";

constexpr std::size_t kFileHeaderOffset = 4;
static constexpr bool kSaluteIsLittleEndian = false;
static constexpr bool kCurrentIsLittleEndian = false;

std::tuple<std::size_t, std::size_t> SaluteFlashMemoryTransferImplementor::formatFlashMemoryPageContent(
	uint8_t *aPageBuffer, File &aFile, bool aIsLastChunk)
{
	if (getFlushingState().isFirstChunk()) {  // Reserve 4 bytes to store file size there
		const auto pageBufferInnerOffset = kFileHeaderOffset;
		const auto maxWriteSize = getFlashMemory()->getFlashMemoryGeometry().writeBlockSize - pageBufferInnerOffset;

		if (maxWriteSize > 0) {
			const auto nRead = aFile.read(aPageBuffer + pageBufferInnerOffset, maxWriteSize);

			return {nRead + pageBufferInnerOffset, nRead};
		} else {
			return {0, 0};
		}
	} else {  // Handle as usual
		return FlashMemoryTransferImplementor::formatFlashMemoryPageContent(aPageBuffer, aFile, aIsLastChunk);
	}
}

void SaluteFlashMemoryTransferImplementor::onFileBufferingFinishedPostChunkFlushed(File &, bool aIsLastChunk)
{
	if (!aIsLastChunk) {
		return;  // Nothing to finalize
	}

	if (getFlashMemory() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s uninitialized `Sys::FlashMemory` instance, panicking", kLogPreamble, __func__);
		Sys::panic();
	}

	const auto flashMemoryPageSize = getFlashMemory()->getFlashMemoryGeometry().writeBlockSize;
	std::uint8_t flashMemoryPageBuffer[flashMemoryPageSize] = {0};

	// Read the current page's content
	const auto baseFlashMemoryPageOffset = getFlashMemory()->getFlashMemoryGeometry()
		.convertAddressIntoWriteBlockOffset(getFlushingState().baseFlashMemoryAddress);
	const auto flashMemoryReadResult = getFlashMemory()->readBlock(baseFlashMemoryPageOffset, 0, flashMemoryPageBuffer,
		flashMemoryPageSize);

	if (flashMemoryReadResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s failed to read from flash memory at offset=%d, panicking!", kLogPreamble, __func__,
			baseFlashMemoryPageOffset);
		Sys::panic();
	}

	// Write the file's size at the first `kFileHeaderOffset` bytes of flash page
	Ut::Cont::EndiannessAwareRepresentation<std::uint32_t, kCurrentIsLittleEndian, kSaluteIsLittleEndian>
		endiannessAwareFileSizeRepresentation{getFlushingState().flashMemoryAddress};
	Sys::Logger::write(Sys::LogLevel::Debug, debugTag(),
		 "%s:%s Initializing file header in Salute flash memory file system, the overall file size = %d B",
		 kLogPreamble, __func__, getFlushingState().flashMemoryAddress);
	std::copy(endiannessAwareFileSizeRepresentation.cbeginTarget(), endiannessAwareFileSizeRepresentation.cendTarget(),
		&flashMemoryPageBuffer[0]);
	const auto flashMemoryWriteResult = getFlashMemory()->writeBlock(baseFlashMemoryPageOffset, 0,
		flashMemoryPageBuffer, flashMemoryPageSize);

	if (flashMemoryWriteResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s failed to write from flash memory at offset=%d, panicking!", kLogPreamble, __func__,
			baseFlashMemoryPageOffset);
		Sys::panic();
	}

}

}  // Bft
