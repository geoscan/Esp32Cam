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
/// details specific to Salute drones.
///
/// \details Ensures that SPI flash memory format complies with the following
/// structure:
/// `[file size 4 bytes][file payload N bytes]`.
/// This format is maintained in the following way:
/// - `onFileBufferingFinishedPreBufferRead(...)` ensures 4 byte padding before
///   writing
/// - `onFileBufferingFinishedPostChunkFlushed(...)` rewrites the first page,
///   placing the file's size in the first 4 bytes
class SaluteFlashMemoryTransferImplementor : public FlashMemoryTransferImplementor {
public:
	using FlashMemoryTransferImplementor::FlashMemoryTransferImplementor;

private:
	virtual std::tuple<std::size_t, std::size_t> formatFlashMemoryPageContent(std::uint8_t *aPageBuffer, File &aFile,
		bool aIsLastChunk) override;

	/// \brief If `aIsLastChunk == true`, will rewrite the first page modifying
	/// its so it will contain the file's size.
	void onFileBufferingFinishedPostChunkFlushed(File &aFile, bool aIsLastChunk) override;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_SALUTEFLASHMEMORYTRANSFERIMPLEMENTOR_HPP
