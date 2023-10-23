//
// File.hpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FILE_HPP_
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FILE_HPP_

#include "buffered_file_transfer/storage/FileSystem.hpp"
#include <cstddef>
#include <cstdint>

namespace Bft {

/// \brief Common polymorphic file descriptor, pointer or identifier of a
/// stored state.
struct FileDescriptor {
	union {
		/// nullptr means the descriptor is invalid
		void *raw;

		/// 0 means the descriptor is invalid
		int identifier;
	};

	inline bool isValid()
	{
		return identifier != 0;
	}

	inline void invalidate()
	{
		identifier = 0;
	}

	inline bool isEqual(const FileDescriptor &aFileDescriptor) const
	{
		return raw == aFileDescriptor.raw;
	}
};

/// \brief Generalization over a file-like resource which only provides
/// relevant API.
///
/// \warning The file is not RAII. It is up to the user to close it.
/// \warning Using multiple instances of a file is not safe. However, having
/// them is.
class File final {
public:
	File(FileSystem *aFileSystem, FileDescriptor aFileDescriptor, const char *aFileName);

	inline File():
		File{nullptr, FileDescriptor{nullptr}, ""}
	{
	}

	File(const File &) = default;
	File(File &&) = default;
	File &operator=(const File &) = default;
	File &operator=(File &&) = default;
	~File() = default;

	/// \brief \sa `FileSystem::append`
	inline std::size_t append(const std::uint8_t *aBuffer, std::size_t aBufferSize)
	{
		return fileSystem ? fileSystem->append(fileDescriptor, aBuffer, aBufferSize) : 0;
	}

	inline bool isValid()
	{
		return fileDescriptor.isValid() && fileSystem != nullptr;
	}

	/// \brief \sa `FileSystem::seek`
	inline std::int32_t seek(std::int32_t aOffset, int aOrigin = FileSystem::PositionStart)
	{
		return fileSystem ? fileSystem->seek(fileDescriptor, aOffset, aOrigin) : FileSystem::PositionError;
	}

	/// \brief \sa `fileSystem::read`
	inline std::size_t read(std::uint8_t *aOutBuffer, std::size_t aOutBufferSize)
	{
		return fileSystem ? fileSystem->read(fileDescriptor, aOutBuffer, aOutBufferSize) : 0;
	}

	inline std::uint32_t getCurrentPosition()
	{
		return fileSystem ? fileSystem->seek(fileDescriptor, 0, FileSystem::PositionCurrent) : 0;
	}

	inline std::uint32_t getFileNameHash() const
	{
		return fileNameHash;
	}

	inline FileDescriptor getRawFileDescriptor() const
	{
		return fileDescriptor;
	}

	void close();

private:
	FileSystem *fileSystem;
	FileDescriptor fileDescriptor;
	std::uint32_t fileNameHash;
};

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FILE_HPP_
