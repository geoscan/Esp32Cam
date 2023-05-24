//
// SingleBufferRamFileSystem.hpp
//
// Created on: May 22, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SINGLEBUFFERRAMFILESYSTEM_HPP
#define BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SINGLEBUFFERRAMFILESYSTEM_HPP

#include "buffered_file_transfer/storage/FileSystem.hpp"

namespace Bft {

/// \brief Hides plain `malloc` under FS API
class SingleBufferRamFileSystem : public FileSystem {
public:
	FileDescriptor tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint) override;
	void closeFile(FileDescriptor aFileDescriptor) override;
	std::size_t append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
		std::size_t aBufferSize) override;
	std::int32_t seek(FileDescriptor aFileDescriptor, std::int32_t aOffset, int aOrigin) override;
	std::size_t read(FileDescriptor aFileDescriptor, std::uint8_t *aOutBuffer, std::size_t &aOutBufferSize) override;
};

}  // namespace Bft

#endif // BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SINGLEBUFFERRAMFILESYSTEM_HPP
