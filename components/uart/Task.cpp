//
// Task.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/Buffer.hpp"
#include "UartDevice.hpp"
#include "Task.hpp"

using namespace Uart;
using namespace Utility;

void Uart::Task::operator()()
{
	while (true) {
		for (auto &uartDevice : uartDevices) {
			auto nRead = uartDevice.read(buffer.data(), buffer.size());

			if (nRead) {
				Subscription::UartMessage uartMessage;
				uartMessage.payload = decltype(uartMessage.payload){buffer.data(), nRead};
				uartMessage.uartNum = static_cast<Subscription::UartNum>(uartDevice.getNum());

				switch (uartMessage.uartNum) {
					case Subscription::UartNum::Mavlink:
						Subscription::Key::MavlinkUartReceived::notify(uartMessage);
						break;

					default:
						break;
				}
			}
		}
	}
}
