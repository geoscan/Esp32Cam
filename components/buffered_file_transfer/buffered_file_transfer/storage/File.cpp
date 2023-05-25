//
// File.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/thr/WorkQueue.hpp"
#include "wifi_uart_bridge/Receiver.hpp"

#include "File.hpp"

namespace Bfl {

}  // namespace Bfl

std::uint32_t Bft::File::getSize()
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

void Bft::File::close()
{
	if (fileDescriptor.isValid() && fileSystem) {
		fileSystem->closeFile(fileDescriptor);
		fileDescriptor.invalidate();
	}
}
