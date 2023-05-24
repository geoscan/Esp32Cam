//
// FileSystem.hpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FILESYSTEM_HPP_
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FILESYSTEM_HPP_

#include <cstddef>
#include <cstdint>
#include <stdio.h>

namespace Bft {

class FileDescriptor;

class FileSystem {
public:
	enum Position : int {
		PositionStart = SEEK_SET,
		PositionCurrent = SEEK_CUR,
		PositionEnd = SEEK_END,
		PositionError = -4,
	};

public:
	/// \brief Expected to open an existing file, or create a new one, if the
	/// file with this name does not yet exist.
	///
	/// \param `aFileSizeHint` is added with keeping the possibility to use
	/// RAM-based FSs in mind
	virtual FileDescriptor tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint) = 0;

	/// \brief Invokes underlying file system to close the file using its `fd`
	virtual void closeFile(FileDescriptor aFileDescriptor) = 0;

	/// \brief Appends bytes at the end of file
	/// \returns `aBufferSize`, if succeeded. 0 otherwise
	virtual std::size_t append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
		std::size_t aBufferSize) = 0;

	/// \brief Changes the file's current position
	/// \returns The new position in a file (a positive integer) relative to
	/// the origin, `PositionError` in case of an error.
	virtual std::int32_t seek(FileDescriptor aFileDescriptor, std::int32_t aOffset, int aOrigin = PositionStart) = 0;

	/// \returns the number of read bytes
	virtual std::size_t read(FileDescriptor aFileDescriptor, std::uint8_t *aOutBuffer, std::size_t aOutBufferSize) = 0;
};

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FILESYSTEM_HPP_
