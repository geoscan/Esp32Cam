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
	memoryParitionTable{aMemoryPartitionTable}
{
}

void FlashMemoryTransferImplementor::onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk)
{
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

	// Write file chunk-by-chunk
	// TODO
}

}  // Bft
