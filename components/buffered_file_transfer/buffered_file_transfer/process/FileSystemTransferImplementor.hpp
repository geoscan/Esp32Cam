//
// FileSystemTransferImplementor.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_FILESYSTEMTRANSFERIMPLEMENTOR_HPP_
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_FILESYSTEMTRANSFERIMPLEMENTOR_HPP_

#include "buffered_file_transfer/process/TransferImplementor.hpp"
#include "buffered_file_transfer/storage/FileSystem.hpp"
#include <cstdint>

namespace Bft {

/// \brief Implements buffer flushing onto a flash memory device
class FileSystemTransferImplementor : public TransferImplementor {
public:
	FileSystemTransferImplementor(FileSystem *aFileSystem);
	void onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk) override;

private:
	FileSystem *fileSystem;
	FileDescriptor fileDescriptor;
};

}  // Bft

#endif  // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_PROCESS_FILESYSTEMTRANSFERIMPLEMENTOR_HPP_
