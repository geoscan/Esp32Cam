//
// FlashMemoryTransferImplementor.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP

#include "buffered_file_transfer/process/TransferImplementor.hpp"
#include "buffered_file_transfer/storage/FlashMemoryPartitionTable.hpp"
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

		/// \brief Prevents the same block from being erased twice.
		std::uint32_t lastErasedBlockOffset{0};

		/// Stage marker. Valid account multiple invokes to
		/// `onFileBufferingFinished`
		bool ongoing{false};

		/// \brief Will rollback to the initial state
		void reset()
		{
			*this = FlushingState{};
		}

		bool isFirstChunk() const
		{
			return baseFlashMemoryAddress == flashMemoryAddress;
		}
	};

public:
	FlashMemoryTransferImplementor(Sys::FlashMemory *aFlashMemory,
		const FlashMemoryPartitionTable *aMemoryParitionTable);
	void onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk) override;

	inline void setFlashMemoryInstance(Sys::FlashMemory *aFlashMemory)
	{
		flashMemory = aFlashMemory;
	}

protected:
	const FlushingState &getFlushingState()
	{
		return flushingState;
	}

	Sys::FlashMemory *getFlashMemory()
	{
		return flashMemory;
	}

	/// \brief Initializes `aPageBuffer`.
	/// \returns tuple of 2: (1) Number of bytes written into `aPageBuffer`,
	/// (2) number of bytes read from `aFile`
	/// \pre FlushingState::flashMemoryAddress is GUARANTEED to point at the
	/// next write address
	/// \pre `getFlashMemory()` is GUARANTEED to return valid pointer
	virtual std::tuple<std::size_t, std::size_t> formatFlashMemoryPageContent(std::uint8_t *aPageBuffer, File &aFile,
		bool aIsLastChunk);

private:
	/// \brief General policy: whether pre-erase should be performed
	bool shouldEraseMemoryBeforeWriting() const;

	/// \brief using `FlushingState` and `flashMemory`, checks whether
	/// erase is needed.
	bool shouldEraseCurrentFlashMemoryBlock() const;

	/// \brief Makes an attempt to erase current flash memory block
	/// \returns true, if success, false otherwise
	bool tryEraseCurrentFlashMemoryBlock();

	/// \brief Reads the current flash memory page's content into `aPageBuffer`
	/// \pre Size of `aPageBuffer` MUST match the actual flash memory page
	/// size
	bool tryReadCurrentFlashMemoryPageContent(std::uint8_t *aPageBuffer);

	/// \brief Writes `aPageBuffer` into the current flash memory page
	/// \pre Size of `aPageBuffer` MUST match the actual flash memory page
	/// size
	bool tryWriteIntoCurrentFlashMemoryPage(const std::uint8_t *aPageBuffer);

	/// \brief Stage hook. Added to perform memory layout-related updates.
	/// Implementors MUST NOT assume ownership of `aBuffer`, or `aFile`.
	/// \pre `baseFlashMemoryAddress` and `flashMemoryAddress` are GUARANTEED
	/// to be correctly initialized, and representative of the current state
	/// \pre `flushingState.flashMemoryAddress` is GUARANTEED to store the
	/// accumulated offset.
	virtual void onFileBufferingFinishedPostChunkFlushed(File &aFile, bool aIsLastChunk);

private:
	Sys::FlashMemory *flashMemory;
	const FlashMemoryPartitionTable *memoryPartitionTable;
	FlushingState flushingState;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP
