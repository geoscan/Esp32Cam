//
// FlashMemoryTransferImplementor.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP

#include "buffered_file_transfer/process/TransferImplementor.hpp"
#include "buffered_file_transfer/storage/MemoryPartitionTable.hpp"
#include "system/middleware/FlashMemory.hpp"
#include "utility/cont/Buffer.hpp"

namespace Bft {

class FlashMemoryTransferImplementor : public TransferImplementor {
protected:
	/// \brief Provides context for current flushing iteration
	struct FlushingState {
		/// The beginning of a file in the flash memory. Valid across multiple
		/// invokes to `onFileBufferingFinished`
		std::uint32_t baseFlashMemoryAddress{0};

		/// \brief Current write address. Valid across multiple invokes to
		/// `onFileBufferingFinished`
		std::uint32_t flashMemoryAddress{0};

		/// Stage marker. Valid account multiple invokes to
		/// `onFileBufferingFinished`
		bool ongoing{false};

		bool isFirstChunk() const
		{
			return baseFlashMemoryAddress == flashMemoryAddress;
		}
	};

public:
	FlashMemoryTransferImplementor(Sys::FlashMemory *aFlashMemory,
		const MemoryPartitionTable *aMemoryParitionTable);
	void onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk) override;

protected:
	FlushingState &getFlushingState()
	{
		return flushingState;
	}

private:
	bool shouldEraseMemoryBeforeWriting() const;

	/// \brief Stage hook. Performs initiation sequence on the first chunk.
	/// Added for taking account for memory layouts. Implementors MUST NOT
	/// assume ownership of `aBuffer`, or `aFile`.
	/// \pre `baseFlashMemoryAddress` and `flashMemoryAddress` are GUARANTEED
	/// to be correctly initialized, and representative of the current state
	/// \pre The buffer is GUARANTEED to contain the page's content
	virtual void onFileBufferingFinishedPreBufferRead(Ut::Cont::Buffer &aBuffer, File &aFile, bool aIsLastChunk);

	/// \brief Stage hook. Added to perform memory layout-related updates.
	/// Implementors MUST NOT assume ownership of `aBuffer`, or `aFile`.
	/// \pre `baseFlashMemoryAddress` and `flashMemoryAddress` are GUARANTEED
	/// to be correctly initialized, and representative of the current state
	virtual void onFileBufferingFinishedPostChunkFlushed(File &aFile, bool aIsLastChunk);

private:
	Sys::FlashMemory *flashMemory;
	const MemoryPartitionTable *memoryPartitionTable;
	FlushingState flushingState;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP
