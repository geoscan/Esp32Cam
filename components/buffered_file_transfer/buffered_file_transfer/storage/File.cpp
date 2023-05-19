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

void Bft::File::finalize()
{
	// TODO: notification
	if (fileDescriptor.isValid()) {
		fileSystem->close(fileDescriptor);
		fileDescriptor.invalidate();
	}
}
