//
// UartTask.hpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UART_UARTTASK_HPP
#define UART_UARTTASK_HPP

#include <list>
#include <array>
#include "utility/Subscription.hpp"

class UartDevice;

namespace Uart {

class Task {
public:
	template <class ...Tuarts>
	Task(Tuarts &&...aArgs);

	void operator()();

private:
	static constexpr unsigned kBufferSize = 512;
	std::array<std::uint8_t, kBufferSize> buffer;
	std::list<UartDevice *> uartDevices;
};

}  // namespace Uart

#include "Task.impl"

#endif  // UART_UARTTASK_HPP
