//
// SingleBufferRamFileSystem.hpp
//
// Created on: May 22, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SINGLEBUFFERRAMFILESYSTEM_HPP
#define BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SINGLEBUFFERRAMFILESYSTEM_HPP

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "buffered_file_transfer/storage/File.hpp"
#include "buffered_file_transfer/storage/FileSystem.hpp"
#include <vector>

namespace Bft {

/// \brief Hides plain `malloc` under FS API. Only one buffer may be used at a
/// time. A multibuffer scheme will requre a similar, but separate
/// implementation.
class SingleBufferRamFileSystem : public FileSystem {
public:
	SingleBufferRamFileSystem():
		memoryPool{nullptr},
		memoryPoolSize{0},
		memoryPoolNextWritePosition{0},
		memoryPoolCurrentPosition{0}
	{
	}

	FileDescriptor tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint) override;
	void closeFile(FileDescriptor aFileDescriptor) override;
	std::size_t append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
		std::size_t aBufferSize) override;
	std::int32_t seek(FileDescriptor aFileDescriptor, std::int32_t aOffset, int aOrigin) override;
	std::size_t read(FileDescriptor aFileDescriptor, std::uint8_t *aOutBuffer, std::size_t aOutBufferSize) override;

private:
	/// \brief Allocated memory pool
	std::uint8_t *memoryPool;

	std::size_t memoryPoolSize;

	/// \brief Its current size is treated as accumulated buffer size. Each
	/// `append()` has the opportunity to increase it using `max()` buffer.
	/// `read()` operation may not access the memory further than that.
	std::size_t memoryPoolNextWritePosition;

	/// \brief Current read / write position
	/// \pre GUARANTEED to be less than `memoryPoolNextWritePosition`
	std::size_t memoryPoolCurrentPosition;
};

}  // namespace Bft

#endif // BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_SINGLEBUFFERRAMFILESYSTEM_HPP
