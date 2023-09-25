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

namespace Bft {

class FlashMemoryTransferImplementor : public TransferImplementor {
private:
	struct FlushingState {
		std::uint32_t baseAddress{0};
		bool ongoing{false};
	};

public:
	FlashMemoryTransferImplementor(Sys::FlashMemory *aFlashMemory,
		const MemoryPartitionTable *aMemoryParitionTable);
	void onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk) override;

private:
	Sys::FlashMemory *flashMemory;
	const MemoryPartitionTable *memoryParitionTable;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FLASHMEMORYTRANSFERIMPLEMENTOR_HPP
