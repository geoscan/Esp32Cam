//
// File.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "wifi_uart_bridge/Receiver.hpp"
#include "utility/al/Crc32.hpp"

#include "File.hpp"

namespace Bft {

File::File(FileSystem *aFileSystem, FileDescriptor aFileDescriptor, const char *aFileName) :
   fileSystem{aFileSystem},
   fileDescriptor{aFileDescriptor},
   fileNameHash{Ut::Al::Crc32::calculateCrc32(static_cast<const void *>(aFileName), strlen(aFileName))}
{
}

void File::close()
{
	if (fileDescriptor.isValid() && fileSystem) {
		fileSystem->closeFile(fileDescriptor);
		fileDescriptor.invalidate();
		fileSystem = nullptr;
	}
}

}  // namespace Bft
