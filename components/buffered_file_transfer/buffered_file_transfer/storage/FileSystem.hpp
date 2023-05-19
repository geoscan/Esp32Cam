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

namespace Bft {

class FileDescriptor;

class FileSystem {
public:
	/// \brief Expected to open an existing file, or create a new one, if the
	/// file with this name does not yet exist.
	///
	/// \param `aFileSizeHint` is added with keeping the possibility to use
	/// RAM-based FSs in mind
	virtual FileDescriptor tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint) = 0;

	/// \brief Invokes underlying file system to close the file using its `fd`
	virtual void closeFile(FileDescriptor aFileDescriptor) = 0;

	/// \returns `aBufferSize`, if succeeded. 0 otherwise
	virtual std::size_t append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
		std::size_t aBufferSize) = 0;
};

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_FILESYSTEM_HPP_
