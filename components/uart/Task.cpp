//
// Task.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sub/Rout.hpp"
#include "utility/Buffer.hpp"
#include "UartDevice.hpp"
#include "Task.hpp"

using namespace Uart;
using namespace Utility;

void Uart::Task::operator()()
{
	while (true) {
		for (auto &uartDevice : uartDevices) {
			auto nRead = uartDevice->read(buffer.data(), buffer.size());

			for (auto &callable : Sub::Rout::OnReceived::getIterators()) {
				auto response = callable(Sub::Rout::Uart{{buffer.data(), nRead}, uartDevice->getNum()});

				if (response.getType() == Sub::Rout::Response::Type::Response) {
					uartDevice->write(response.payload);
				}
			}
		}
	}
}
