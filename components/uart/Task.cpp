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
#include <cassert>
#include "utility/CircularSwap.hpp"
#include "utility/Buffer.hpp"
#include "uart/uart.hpp"
#include "sub/Rout.hpp"
#include "utility/system/UartDevice.hpp"
#include "Task.hpp"
#include "utility/time.hpp"
#include "utility/thr/WorkQueue.hpp"
#include "wifi_uart_bridge/Receiver.hpp"

#if 0
# define UART_SECTION_LOGV(tag, title) GS_UTILITY_LOG_SECTIONV(tag, title)
#else
# define UART_SECTION_LOGV(tag, title)
#endif  // #if 0

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Uart::Task, taskProcess, 0)
GS_UTILITY_LOGD_METHOD_SET_ENABLED(Uart::Task, taskProcess, 0)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Uart::Task, taskRead, 0)
GS_UTILITY_LOGD_METHOD_SET_ENABLED(Uart::Task, taskRead, 0)

enum class LogAspect {
	BufferManagement,
};

GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Uart::Task, LogAspect::BufferManagement, 0)

using namespace Uart;
using namespace Utility;

void Task::taskRead()
{
	ESP_LOGI(Uart::kDebugTag, "Task::taskRead - started");

	while (true) {
		for (auto uart : uartDevices) {
			auto *buffer = swap.getFree();

			if (nullptr != buffer) {
				buffer->device = uart;
				buffer->pos = uart->read(buffer->buf.data(), buffer->buf.size());

				if (buffer->pos) {
					GS_UTILITY_LOGV_METHOD(Uart::kDebugTag, Uart::Task, taskRead, "uart %d got %d bytes",
						uart->getNum(), buffer->pos);
					swap.pushFull(buffer);
				} else {
					swap.pushFree(buffer);
					Utility::waitMs(20);
				}

				GS_UTILITY_LOG_EVERY_N_TURNS(100, GS_UTILITY_LOGD_CLASS_ASPECT(Uart::kDebugTag, Uart::Task,
					LogAspect::BufferManagement, "free buffers %d", swap.swap.countFree());)
			} else {
				ESP_LOGW(Uart::kDebugTag, "taskRead(): Could not pop a free buffer");
				Utility::waitMs(20);
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
			GS_UTILITY_LOGV_METHOD(Uart::kDebugTag, Task, taskProcess, "Got buffer of %d bytes to process",
				buffer->pos);
			Bdg::Receiver::notifyAs({Bdg::UartEndpoint{static_cast<std::uint8_t>(buffer->device->getNum()), {}},
				{buffer->buf.data(), buffer->pos},
				[&buffer](Bdg::RespondCtx aCtx)
				{
					GS_UTILITY_LOGD_METHOD(Uart::kDebugTag, Task, taskProcess, "got response of %d bytes",
						aCtx.buffer.size());
					buffer->device->write(aCtx.buffer);
				}});
			swap.pushFree(buffer);  // The buffer has been processed
		} else {
			Utility::waitMs(20);  // To prevent resource starvation.
		}
	}
}

Task::Buf *Task::SyncedSwap::getFree()
{
	GS_UTILITY_LOGV_METHOD_SECTION(Uart::kDebugTag, Uart::Task::SyncedSwap, getFree);
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	Task::Buf *ret = nullptr;

	if (swap.countFree()) {
		ret = &swap.popFree();
	} else if (swap.countFull()) {
		ret = &swap.popFull();  // It's better to trash a stalled buffer than miss a new message
	}

	return ret;
}

Task::Buf *Task::SyncedSwap::getFull()
{
	GS_UTILITY_LOGV_METHOD_SECTION(Uart::kDebugTag, Uart::Task::SyncedSwap, getFull);
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	Task::Buf *ret = nullptr;

	if (swap.countFull()) {
		ret = &swap.popFull();
	}

	return ret;
}

void Task::SyncedSwap::pushFree(Task::Buf *aBuf)
{
	GS_UTILITY_LOGV_METHOD_SECTION(Uart::kDebugTag, Uart::Task::SyncedSwap, pushFree);
	assert(nullptr != aBuf);
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	swap.pushFree(*aBuf);
}

void Task::SyncedSwap::pushFull(Task::Buf *aBuf)
{
	GS_UTILITY_LOGV_METHOD_SECTION(Uart::kDebugTag, Uart::Task::SyncedSwap, pushFull);
	assert(nullptr != aBuf);
	UART_SECTION_LOGV(Uart::kDebugTag, "Task::SyncedSwap::pushFull()");
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;

	swap.pushFull(*aBuf);
}

#undef UART_SECTION_LOGV
