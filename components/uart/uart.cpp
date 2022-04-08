//
// uart.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_UART_DEBUG_LEVEL)
#include <esp_log.h>

#include "utility/CircularSwap.hpp"
#include "uart/uart.hpp"
#include "UartDevice.hpp"
#include "Task.hpp"
#include "utility/Threading.hpp"
#include "OnSend.hpp"

#include <thread>

namespace Uart {

static Task *task = nullptr;

void init()
{
	static UartDevice devices[] = {
		UartDevice{UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, CONFIG_WIFI_UART_BRIDGE_BAUDRATE, UART_PARITY_DISABLE,
			UART_STOP_BITS_1}  // MAVLink / Autopilot link
	};
	static Task sTask{devices[0]};
	static OnSend<1> onSend{devices[0]};
	(void)onSend;
	task = &sTask;
}

void start()
{
	init();

	ESP_LOGD(Uart::kDebugTag, "Uart::start. starting UART process task");
	Utility::Threading::Config(true).core(0).stack(CONFIG_ESP32_PTHREAD_TASK_STACK_SIZE_DEFAULT + 2048);
	static std::thread threadProcess{&Task::taskProcess, task};
	(void)threadProcess;
	ESP_LOGD(Uart::kDebugTag, "Uart::start. started UART process task");

	ESP_LOGD(Uart::kDebugTag, "Uart::start. starting UART read task");
	Utility::Threading::Config(true).core(0).stack(CONFIG_ESP32_PTHREAD_TASK_STACK_SIZE_DEFAULT - 1024);
	static volatile std::thread threadRead{&Task::taskRead, task};
	(void)threadRead;
	ESP_LOGD(Uart::kDebugTag, "Uart::start. started UART read task");
}

}  // namespace Uart
