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
	const FlashMemoryPartitionTable *aMemoryPartitionTable):
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

	// Erase memory chunk, if starting from scratch, and erasing is required

	if (!flushingState.ongoing) {
		if (!memoryPartitionTable->tryGetFlashMemoryAddressByFile(*aFile.get(), flushingState.baseFlashMemoryAddress)) {
			Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
				"%s:%s failed to get memory address by file name, skipping", kLogPreamble, __func__);

			return;
		}

		flushingState.flashMemoryAddress = flushingState.baseFlashMemoryAddress;
	}

	// Make sure we can write over: perform pre-erase
	if (shouldEraseCurrentFlashMemoryBlock()) {
		if (!tryEraseCurrentFlashMemoryBlock()) {
			Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
				"%s:%s failed to erase current memory block, ignoring", kLogPreamble, __func__);  // TODO: critical fail?
		}
	}

	// Write file chunk-by-chunk
	flushingState.ongoing = true;

	const auto bufferSize = aFile->getCurrentPosition();
	Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(), "%s:%s the file's current size is %d", kLogPreamble,
		__func__, bufferSize);
	aFile->seek(0);  // Reset for reading from the beginning

	// convert current address into page id.
	const auto pageSize = flashMemory->getFlashMemoryGeometry().writeBlockSize;
	std::uint8_t pageBuffer[pageSize] = {0xFF};
	std::size_t nProcessedBufferBytesTotal = 0;
	std::size_t nProcessedBufferBytesIteration = 0;

	for (;nProcessedBufferBytesTotal != bufferSize; nProcessedBufferBytesTotal += nProcessedBufferBytesIteration) {
		// Make sure we can write over: perform pre-erase
		if (shouldEraseCurrentFlashMemoryBlock()) {
			if (!tryEraseCurrentFlashMemoryBlock()) {
				Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
					"%s:%s failed to erase current memory block, ignoring", kLogPreamble, __func__);  // TODO: critical fail?
			}
		}

		// Size validation
		if (nProcessedBufferBytesTotal > bufferSize) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
				"%s:%s the length of the written chunk (%d B) has exceeded the buffer size (%d B), aborting!",
					kLogPreamble, __func__);
			aIsLastChunk = true;

			break;
		}

		Ut::Cont::Buffer pageBufferSpan{pageBuffer, pageSize};  // Stage handlers may slice the buffer (advance the pointer to leave some space for further markup management)

		if (!tryReadCurrentFlashMemoryPageContent(pageBuffer)) {  // TODO: is pre-read necessary
			return;
		}

		const auto pageOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoWriteBlockOffset(
			flushingState.flashMemoryAddress);

		// Handle the buffer, slice if needed
		onFileBufferingFinishedPreBufferRead(pageBufferSpan, *aFile.get(), aIsLastChunk);

		// Read from buffer
		nProcessedBufferBytesIteration = aFile->read(static_cast<std::uint8_t *>(pageBufferSpan.data()), pageBufferSpan.size());

		if (nProcessedBufferBytesIteration == 0) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to read from buffer %s panicking!",
				kLogPreamble, __func__, aIsLastChunk ? "(last chunk)" : "");
			Sys::panic();
		}

		// Try to flush the buffer into flash memory
		if (!tryWriteIntoCurrentFlashMemoryPage(pageBuffer)) {
			return;
		}

		flushingState.flashMemoryAddress += pageSize;
		Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(),
			"%s:%s wrote %d B during the current flushing iteration, overall %d B", kLogPreamble, __func__,
			nProcessedBufferBytesTotal, flushingState.flashMemoryAddress);
	}

	onFileBufferingFinishedPostChunkFlushed(*aFile.get(), aIsLastChunk);

	if (aIsLastChunk) {
		Sys::Logger::write(Sys::LogLevel::Debug, debugTag(), "%s:%s finalizing write, wrote %d B overall",
			kLogPreamble, __func__, nProcessedBufferBytesTotal);
		flushingState.reset();
	}
}

bool FlashMemoryTransferImplementor::shouldEraseMemoryBeforeWriting() const
{
	return true;
}

bool FlashMemoryTransferImplementor::shouldEraseCurrentFlashMemoryBlock() const
{
	if (shouldEraseMemoryBeforeWriting()) {
		auto eraseBlockOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoEraseBlockOffset(
			flushingState.flashMemoryAddress);

		if (!flushingState.ongoing // The process has just started
			|| flushingState.lastErasedBlockOffset != eraseBlockOffset  // This block has not been erased yet
		) {
			return true;
		}
	}

	return false;
}

bool FlashMemoryTransferImplementor::tryEraseCurrentFlashMemoryBlock()
{
	auto eraseBlockOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoEraseBlockOffset(
		flushingState.flashMemoryAddress);
	const auto eraseResult = flashMemory->eraseBlock(eraseBlockOffset);
	flushingState.lastErasedBlockOffset = eraseBlockOffset;

	if (eraseResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s failed to erase memory, ignoring",  // TODO: abort the process
			kLogPreamble, __func__);

		return false;
	} else {
		Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(), "%s:%s successfully erase flash memory block # %d",
			kLogPreamble, __func__, eraseBlockOffset);

		return true;
	}

}

bool FlashMemoryTransferImplementor::tryReadCurrentFlashMemoryPageContent(uint8_t *aPageBuffer)
{
	const auto pageOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoWriteBlockOffset(
		flushingState.flashMemoryAddress);
	const auto pageSize = flashMemory->getFlashMemoryGeometry().writeBlockSize;
	constexpr std::uint32_t kInnerPageOffset = 0;
	const auto flashMemoryReadResult = flashMemory->readBlock(pageOffset, kInnerPageOffset, aPageBuffer, pageSize);

	if (flashMemoryReadResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s failed to read from memory, skipping further handling", kLogPreamble, __func__);

		return false;
	}

	return true;
}

bool FlashMemoryTransferImplementor::tryWriteIntoCurrentFlashMemoryPage(const uint8_t *aPageBuffer)
{
	const auto pageOffset = flashMemory->getFlashMemoryGeometry().convertAddressIntoWriteBlockOffset(
		flushingState.flashMemoryAddress);
	const auto pageSize = flashMemory->getFlashMemoryGeometry().writeBlockSize;
	const auto writeResult = flashMemory->writeBlock(pageOffset, 0,
		aPageBuffer, pageSize);

	if (writeResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s failed to write into flash memory (\"%s\"), aborting", kLogPreamble, __func__,
			writeResult.description);

		return false;
	}

	return true;
}

void FlashMemoryTransferImplementor::onFileBufferingFinishedPreBufferRead(Ut::Cont::Buffer &, File &, bool)
{
}

void FlashMemoryTransferImplementor::onFileBufferingFinishedPostChunkFlushed(File &, bool)
{
}

}  // Bft
