//
// BufferedFileTranslfer.hpp
//
// Created on: May 18, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_BUFFEREDFILETRANSLFER_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_BUFFEREDFILETRANSLFER_HPP

#include "buffered_file_transfer/storage/File.hpp"
#include "buffered_file_transfer/storage/FileSystem.hpp"
#include "utility/MakeSingleton.hpp"
#include <cstddef>

namespace Bft {

class BufferedFileTransfer final : public Ut::MakeSingleton<BufferedFileTransfer> {
public:
	inline BufferedFileTransfer(FileSystem &aFileSystem) :
		fileSystem{&aFileSystem}
	{
	}

	/// \brief Creates a new file descriptor invoking the file system's API it
	/// has been provided with
	/// \param `aFileSize`. Depending on the file system in use, it may or may
	/// not be used.
	inline File tryOpenFileWriteBinary(const char *aFileName, std::size_t aFileSize);

private:
	FileSystem *fileSystem;
};

}  // namespace Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_BUFFEREDFILETRANSLFER_HPP
