//
// BufferedFileTransfer.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "BufferedFileTransfer.hpp"

namespace Bft {

File BufferedFileTransfer::tryOpenFileWriteBinary(const char *aFileName, std::size_t aFileSize)
{
	FileDescriptor fileDescriptor {nullptr};

	if (fileSystem != nullptr) {
		fileDescriptor = fileSystem->tryOpenFileWriteBinary(aFileName, aFileSize);
	}

	return {fileSystem, fileDescriptor};
}

}  // namespace Bft
