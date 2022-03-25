//
// Task.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_UART_DEBUG_LEVEL)
#include <esp_log.h>

#include "uart/uart.hpp"
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
						buf = response.nProcessed > 0 && response.nProcessed < buf.size() ?
						buf.slice(response.nProcessed) : buf.slice(buf.size()))
					{
						ESP_LOGV(Uart::kDebugTag, "Task::operator(): processing (%d bytes remain)", buf.size());
						response = callable(Sub::Rout::Uart{Utility::makeAsioCb(buf), uartDevice->getNum()});
						ESP_LOGV(Uart::kDebugTag, "Task::operator(): chunk nProcessed %d", response.nProcessed);

						if (Sub::Rout::Response::Type::Response == response.getType()) {
							ESP_LOGV(Uart::kDebugTag, "Task::operator(): send response size %d",
								response.payload.size());
							uartDevice->write(response.payload);
						}
					}
				}
			}
		}
		Utility::waitMs(20);
	}
}
