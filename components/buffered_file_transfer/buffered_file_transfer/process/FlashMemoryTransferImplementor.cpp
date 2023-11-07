//
// FlashMemoryTransferImplementor.cpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include "utility/al/Algorithm.hpp"

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

	for (std::size_t nProcessedBufferBytesIteration = 0;
		nProcessedBufferBytesTotal != bufferSize;
		nProcessedBufferBytesTotal += nProcessedBufferBytesIteration
	) {
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

		if (!tryReadCurrentFlashMemoryPageContent(pageBuffer)) {  // TODO: is pre-read necessary
			return;
		}

		// Read from buffer
		std::size_t nBytesWrittenIntoPage = 0;
		std::tie(nBytesWrittenIntoPage, nProcessedBufferBytesIteration) = formatFlashMemoryPageContent(pageBuffer,
			*aFile.get(), aIsLastChunk);

		if (shouldVerifyCrc32()) {
			const auto pageBufferInnerOffset = flashMemory->getFlashMemoryGeometry()
				.convertAddressIntoWriteBlockInnerOffset(flushingState.flashMemoryAddress);
			updatePageBufferCrc32(pageBuffer + pageBufferInnerOffset, nBytesWrittenIntoPage);
		}

		if (nProcessedBufferBytesIteration == 0) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to read from buffer %s panicking!",
				kLogPreamble, __func__, aIsLastChunk ? "(last chunk)" : "");
			Sys::panic();
		}

		// Try to flush the buffer into flash memory
		if (!tryWriteIntoCurrentFlashMemoryPage(pageBuffer)) {
			return;
		}

		flushingState.flashMemoryAddress += nBytesWrittenIntoPage;
		Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(),
			"%s:%s wrote %d B during the current flushing iteration, overall %d B", kLogPreamble, __func__,
			nProcessedBufferBytesIteration, nProcessedBufferBytesTotal);
	}

	// Verify CRC32, call before `onFileBufferingFinishedPostChunkFlushed` so we're sure flash memory content is not changed
	if (aIsLastChunk && shouldVerifyCrc32()) {
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s starting CRC32 validation on the last chunk",
			kLogPreamble, __func__);

		updateFlashMemoryCrc32();

		if (flushingState.crc32CalculationState.isMatch()) {
			Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s CRC32 validation has succeeded", kLogPreamble,
				__func__);
		} else {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s CRC32 validation has failed, continuing",
				kLogPreamble, __func__);
		}
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

std::tuple<std::size_t, std::size_t> FlashMemoryTransferImplementor::formatFlashMemoryPageContent(uint8_t *aPageBuffer,
	File &aFile, bool aIsLastChunk)
{
	const auto pageBufferInnerOffset = getFlashMemory()->getFlashMemoryGeometry().convertAddressIntoWriteBlockInnerOffset(
		getFlushingState().baseFlashMemoryAddress);
	const auto maxWriteSize = getFlashMemory()->getFlashMemoryGeometry().writeBlockSize - pageBufferInnerOffset;

	if (maxWriteSize > 0) {
		const auto nProcessed = aFile.read(aPageBuffer + pageBufferInnerOffset, maxWriteSize);

		return {nProcessed, nProcessed};
	} else {
		return {0, 0};
	}
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

void FlashMemoryTransferImplementor::onFileBufferingFinishedPostChunkFlushed(File &, bool)
{
}

void FlashMemoryTransferImplementor::updatePageBufferCrc32(const std::uint8_t *aPageBufferChunk,
	std::size_t aPageBufferChunkLength)
{
	flushingState.crc32CalculationState.bufferReadChecksum.update(aPageBufferChunk, aPageBufferChunkLength);
}

void FlashMemoryTransferImplementor::updateFlashMemoryCrc32(std::uint8_t *aPageCapacityBuffer)
{
	if (!flushingState.ongoing) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
			"%s:%s will not calculate the checksum, because the current base flash memory address cannot be reliable, "
			"skipping", kLogPreamble, __func__);

		return;
	}

	auto flashMemoryReadAddress = flushingState.baseFlashMemoryAddress;
	const auto flashMemoryPageSize = getFlashMemory()->getFlashMemoryGeometry().writeBlockSize;
	bool shouldDeallocateCapacityBuffer = false;  // TODO: handle deallocation

	if (aPageCapacityBuffer == nullptr) {
		shouldDeallocateCapacityBuffer = true;
		aPageCapacityBuffer = new std::uint8_t[flashMemoryPageSize];
	}

	for (flashMemoryReadAddress = flushingState.baseFlashMemoryAddress;
		flashMemoryReadAddress != flushingState.flashMemoryAddress;
	) {
		const auto bufferReadSize = Ut::Al::clamp<std::size_t>(
			flushingState.flashMemoryAddress - flashMemoryReadAddress, 0, flashMemoryPageSize);
		const auto readBlockId = getFlashMemory()->getFlashMemoryGeometry()
			.convertAddressIntoWriteBlockOffset(flashMemoryReadAddress);
		const auto readBlockInnerOffset = getFlashMemory()->getFlashMemoryGeometry()
			.convertAddressIntoWriteBlockInnerOffset(flashMemoryReadAddress);  // Should always be zero. Keeps the code correct in the general case
		const auto flashMemoryReadResult = getFlashMemory()->readBlock(readBlockId, readBlockInnerOffset,
			aPageCapacityBuffer, bufferReadSize);
		flashMemoryReadAddress += bufferReadSize;

		if (flashMemoryReadResult.errorCode != Sys::ErrorCode::None) {
			Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
				"%s:%s failed to read from flash memory, aborting validation", kLogPreamble, __func__);

			break;
		}

		flushingState.crc32CalculationState.flashMemoryReadChecksum.update(aPageCapacityBuffer, bufferReadSize);
	}

	Sys::Logger::write(Sys::LogLevel::Debug, debugTag(), "%s:%s finishing CRC32 validation, processed %d B",
		kLogPreamble, __func__, flashMemoryReadAddress - flushingState.baseFlashMemoryAddress);


	if (shouldDeallocateCapacityBuffer) {
		delete [] aPageCapacityBuffer;
	}
}

inline bool FlashMemoryTransferImplementor::Crc32CalculationState::isMatch() const
{
	return bufferReadChecksum.getValue() == flashMemoryReadChecksum.getValue();
}

}  // Bft