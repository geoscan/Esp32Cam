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
#include "utility/time.hpp"

using namespace Uart;
using namespace Utility;

void Uart::Task::operator()()
{
	while (true) {
		for (auto &uartDevice : uartDevices) {
			auto nRead = uartDevice->read(buffer.data(), buffer.size());

			if (nRead) {
				for (auto &callable : Sub::Rout::OnReceived::getIterators()) {
					typename Sub::Rout::OnReceived::Ret response;

					for (auto buf = Utility::toBuffer<const std::uint8_t>(buffer.data(), nRead); buf.size();
						buf = response.nProcessed ? buf.slice(response.nProcessed) : buf.slice(buf.size()))
					{
						response = callable(Sub::Rout::Uart{Utility::makeAsioCb(buf), uartDevice->getNum()});

						if (Sub::Rout::Response::Type::Response == response.getType()) {
							uartDevice->write(response.payload);
						}
					}
				}
			}
		}
		Utility::waitMs(20);
	}
}
