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
#include "utility/LogSection.hpp"

#if 0
# define UART_SECTION_LOGV(tag, title) GS_UTILITY_LOG_SECTIONV(tag, title)
#else
# define UART_SECTION_LOGV(tag, title)
#endif  // #if 0

#include <cassert>
#include "utility/CircularSwap.hpp"
#include "utility/Buffer.hpp"
#include "utility/LogSection.hpp"
#include "uart/uart.hpp"
#include "sub/Rout.hpp"
#include "UartDevice.hpp"
#include "Task.hpp"
#include "utility/time.hpp"

using namespace Uart;
using namespace Utility;

void Task::taskRead()
{
	ESP_LOGI(Uart::kDebugTag, "Task::taskRead - started");
	unsigned logCounter = 0;

	while (true) {
		for (auto uart : uartDevices) {
			auto &buffer = swap.getFree();

			buffer.device = uart;
			buffer.pos = uart->read(buffer.buf.data(), buffer.buf.size());

			if (buffer.pos) {
				ESP_LOGV(Uart::kDebugTag, "uart %d got %d bytes", uart->getNum(), buffer.pos);
				swap.pushFull(buffer);
			} else {
				swap.pushFree(buffer);
				Utility::waitMs(20);
			}

			if ((logCounter = (logCounter + 1) % 100) == 0) {
				ESP_LOGD(Uart::kDebugTag, "Task::taskRead: free buffers %d", swap.swap.countFree());
			}
		}
	}
}

void Task::taskProcess()
{
	ESP_LOGI(Uart::kDebugTag, "Task::taskProcess - started");

	while (true) {
		auto *buffer = swap.getFull();

		if (nullptr != buffer) {
			ESP_LOGV(Uart::kDebugTag, "got buffer to process");
			for (auto &callable : Sub::Rout::OnReceived::getIterators()) {
				typename Sub::Rout::OnReceived::Ret response;

				// Process buffer chunk-by-chunk
				for (auto bufView = Utility::toBuffer<const std::uint8_t>(buffer->buf.data(), buffer->pos);
					bufView.size();
					bufView = response.nProcessed > 0 && response.nProcessed < bufView.size() ?
					bufView.slice(response.nProcessed) :
					bufView.slice(bufView.size()))
				{
					ESP_LOGV(Uart::kDebugTag, "Task::operator(): processing (%d bytes remain)", bufView.size());
					response = callable(Sub::Rout::Uart{Utility::makeAsioCb(bufView), buffer->device->getNum()});
					ESP_LOGV(Uart::kDebugTag, "Task::operator(): chunk nProcessed %d", response.nProcessed);

					if (Sub::Rout::Response::Type::Response == response.getType()) {  // Have something to return to the client
						ESP_LOGV(Uart::kDebugTag, "Task::operator(): send response size %d", response.payload.size());
						buffer->device->write(response.payload);
					}

					response.reset();  // Free mutexes or buffers stored in the `Response` object
				}
			}

			swap.pushFree(*buffer);  // The buffer has been processed
		} else {
			Utility::waitMs(20);  // To prevent resource starvation.
		}
	}
}

Task::Buf &Task::SyncedSwap::getFree()
{
	UART_SECTION_LOGV(Uart::kDebugTag, "Task::SyncedSwap::getFree()");
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	if (swap.countFree()) {
		return swap.popFree();
	} else {
		return swap.popFull();
	}
}

Task::Buf *Task::SyncedSwap::getFull()
{
	UART_SECTION_LOGV(Uart::kDebugTag, "Task::SyncedSwap::getFull()");
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	if (swap.countFull()) {
		return &swap.popFull();
	}

	return nullptr;
}

void Task::SyncedSwap::pushFree(Task::Buf &aBuf)
{
	UART_SECTION_LOGV(Uart::kDebugTag, "Task::SyncedSwap::pushFree()");
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	swap.pushFree(aBuf);
}

void Task::SyncedSwap::pushFull(Task::Buf &aBuf)
{
	UART_SECTION_LOGV(Uart::kDebugTag, "Task::SyncedSwap::pushFull()");
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	swap.pushFull(aBuf);
}

#undef UART_SECTION_LOGV
