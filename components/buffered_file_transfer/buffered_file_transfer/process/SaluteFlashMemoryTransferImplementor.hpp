//
// SaluteFlashMemoryTransferImplementor.hpp
//
// Created on: Sep 27, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_SALUTEFLASHMEMORYTRANSFERIMPLEMENTOR_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_SALUTEFLASHMEMORYTRANSFERIMPLEMENTOR_HPP

#include "buffered_file_transfer/process/FlashMemoryTransferImplementor.hpp"

namespace Bft {

/// \brief Performs memory management taking account of the memory management
/// details specific to Salute drones
class SaluteFlashMemoryTransferImplementor : public FlashMemoryTransferImplementor {
private:
	/// \brief Will add a 4-byte spacing in the buffer, which will later be
	/// used for writing file size
	void onFileBufferingFinishedPreBufferRead(Ut::Cont::Buffer &aBuffer, File &aFile, bool aIsLastChunk) override;

	/// \brief If `aIsLastChunk == true`, will rewrite the first page modifying
	/// its so it will contain the file's size.
	void onFileBufferingFinishedPostChunkFlushed(File &aFile, bool aIsLastChunk) override;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_SALUTEFLASHMEMORYTRANSFERIMPLEMENTOR_HPP
