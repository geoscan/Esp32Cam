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
/// stored state
struct FileDescriptor {
	union {
		void *raw;
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
};

/// \brief Generalization over a file-like resource which only provides
/// relevant API
class File final {
public:
	inline File(FileSystem &aFileSystem, FileDescriptor aFileDescriptor) :
		fileSystem{&aFileSystem},
		fileDescriptor{aFileDescriptor}
	{
	}

	inline ~File()
	{
		finalize();
	}

	inline std::size_t append(const std::uint8_t *aBuffer, std::size_t aBufferSize)
	{
		return fileSystem->append(fileDescriptor, aBuffer, aBufferSize);
	}

	inline bool isValid()
	{
		return fileDescriptor.isValid();
	}

	void finalize();

private:
	FileSystem *fileSystem;
	FileDescriptor fileDescriptor;
};

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_FILE_HPP_
