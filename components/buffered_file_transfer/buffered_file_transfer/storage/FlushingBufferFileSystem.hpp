//
// FlushingBufferFileSystem.hpp
//
// Created on: Sep 21, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FLUSHINGBUFFERFILESYSTEM_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FLUSHINGBUFFERFILESYSTEM_HPP

#include "buffered_file_transfer/process/TransferImplementor.hpp"
#include "buffered_file_transfer/storage/File.hpp"
#include "buffered_file_transfer/storage/FileSystem.hpp"
#include "buffered_file_transfer/storage/SingleBufferRamFileSystem.hpp"

namespace Bft {

/// \brief Holds 2 storages: a buffer (RAM), and a flush file system
/// (non-volatile, although the design allows for more exotic options).
/// \details Users should expect delays because of periodic buffer flushes in,
/// for example, non-volatile memory.
/// \warning It's only allowed to have 1 session at a time
class FlushingBufferFileSystem : public FileSystem {
public:
	FlushingBufferFileSystem(FileSystem *aBufferFileSystem);
	virtual FileDescriptor tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint) override;
	virtual std::size_t append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
		std::size_t aBufferSize) override;
	virtual std::int32_t seek(FileDescriptor aFileDescriptor, std::int32_t aOffset,
		int aOrigin) override;
	virtual std::size_t read(FileDescriptor aFileDescriptor, std::uint8_t *aOutBuffer,
		std::size_t aOutBufferSize) override;

private:
	FileSystem *bufferFileSystem;
	FileDescriptor bufferFileDescriptor;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FLUSHINGBUFFERFILESYSTEM_HPP
