//
// uart.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

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
		UartDevice{UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, CONFIG_WIFI_UART_BRIDGE_BAUDRATE, UART_PARITY_DISABLE, UART_STOP_BITS_1}  // MAVLink / Autopilot link
	};
	static Task sTask{devices[0]};
	static OnSend<3> onSend{devices[0]};
	(void)onSend;
	task = &sTask;
}

void start()
{
	init();
	Utility::Threading::Config(true).core(0).stack(CONFIG_ESP32_PTHREAD_TASK_STACK_SIZE_DEFAULT + 1024);
	static std::thread thread{&Task::operator(), task};
}

}  // namespace Uart
