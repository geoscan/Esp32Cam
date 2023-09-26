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
   fileNameHash{static_cast<const void *>(aFileName), strlen(aFileName)}
{
}

std::uint32_t File::getSize()
{
	if (fileSystem) {
		const auto currentPosition = getCurrentPosition();
		const auto endOffset = seek(0, FileSystem::PositionEnd);  // 0 bytes from the end

		if (endOffset != FileSystem::PositionError) {
			seek(currentPosition);

			return static_cast<std::uint32_t>(endOffset);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
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
