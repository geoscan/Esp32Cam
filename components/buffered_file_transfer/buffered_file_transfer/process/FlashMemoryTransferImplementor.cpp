//
// FlashMemoryTransferImplementor.cpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"

#include "FlashMemoryTransferImplementor.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "FlashMemoryTransferImplementor";

FlashMemoryTransferImplementor::FlashMemoryTransferImplementor(Sys::FlashMemory *aFlashMemory,
	const MemoryPartitionTable *aMemoryPartitionTable):
	flashMemory{aFlashMemory},
	memoryPartitionTable{aMemoryPartitionTable}
{
}

void FlashMemoryTransferImplementor::onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk)
{
	// Check pointers
	if (aFile.get() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s `nullptr` file. Unexpected value, panicking!",
			kLogPreamble, __func__);
		Sys::panic();
	}

	if (flashMemory == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s `nullptr` flash memory. Unexpected value, panicking!", kLogPreamble, __func__);
		Sys::panic();
	}

	if (memoryPartitionTable == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s partition table is not initialized", kLogPreamble,
			__func__);
		Sys::panic();
	}

	// Erase memory chunk, if starting from scratch, and erasing required

	if (!flushingState.ongoing) {
		if (!memoryPartitionTable->tryGetFlashMemoryAddressByFile(*aFile.get(), flushingState.baseFlashMemoryAddress)) {
			Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
				"%s:%s failed to get memory address by file name, skipping", kLogPreamble, __func__);
			flushingState.flashMemoryAddress = flushingState.flashMemoryAddress;

			return;
		}
	}

	if (shouldEraseMemoryBeforeWriting() && !flushingState.ongoing) {
		auto eraseBlockOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoEraseBlockOffset(
			flushingState.flashMemoryAddress);
		const auto eraseResult = flashMemory->eraseBlock(eraseBlockOffset);

		if (eraseResult.errorCode != Sys::ErrorCode::None) {
			Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s failed to erase memory, skipping",
				kLogPreamble, __func__);
		}
	}

	// Write file chunk-by-chunk
	flushingState.ongoing = true;

	const auto fileSize = aFile->getSize();
	aFile->seek(0);  // Reset for reading from the beginning
	std::size_t nWrittenTotal = 0;

	// convert current address into page id.
	const auto pageSize = flashMemory->getFlashMemoryGeometry().writeBlockSize;
	std::uint8_t pageBuffer[pageSize] = {0xFF};

	for (std::size_t nRead = 0; nWrittenTotal != fileSize; nWrittenTotal += nRead) {
		Ut::Cont::Buffer pageBufferSpan{pageBuffer, pageSize};  // Stage handlers may slice the buffer (advance the pointer to leave some space for further markup management)
		const auto pageOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoWriteBlockOffset(
			flushingState.flashMemoryAddress);

		// Read the page's content
		const auto flashMemoryReadResult = flashMemory->readBlock(pageOffset, 0, pageBuffer, pageSize);

		if (flashMemoryReadResult.errorCode != Sys::ErrorCode::None) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
				"%s:%s failed to read from memory, skipping further handling", kLogPreamble, __func__);

			return;
		}

		// Handle the buffer, slice if needed
		onFileBufferingFinishedPreBufferRead(pageBufferSpan, *aFile.get(), aIsLastChunk);

		// Read from buffer
		nRead = aFile->read(static_cast<std::uint8_t *>(pageBufferSpan.data()), pageBufferSpan.size());

		if (nRead == 0) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to read from buffer, panicking!",
				kLogPreamble, __func__);
			Sys::panic();
		}

		// Try to flush the buffer into flash memory
		const auto writeResult = flashMemory->writeBlock(pageOffset, 0,
			static_cast<const std::uint8_t *>(pageBufferSpan.data()), pageBufferSpan.size());

		if (writeResult.errorCode != Sys::ErrorCode::None) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to write into flash memory (\"%s\")",
				kLogPreamble, __func__, writeResult.description);

			return;
		}

		nWrittenTotal += nRead;  // The entire read chunk is expected to be written
		flushingState.flashMemoryAddress += nRead;
	}

	onFileBufferingFinishedPostChunkFlushed(*aFile.get(), aIsLastChunk);

	if (aIsLastChunk) {
		flushingState.ongoing = false;
	}
}

bool FlashMemoryTransferImplementor::shouldEraseMemoryBeforeWriting() const
{
	return true;
}

void FlashMemoryTransferImplementor::onFileBufferingFinishedPreBufferRead(Ut::Cont::Buffer &, File &, bool)
{
}

void FlashMemoryTransferImplementor::onFileBufferingFinishedPostChunkFlushed(File &, bool)
{
}

}  // Bft
