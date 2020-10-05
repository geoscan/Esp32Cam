//
// Stm32Sink.hpp
//
// Created on:  Oct 05, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_STM32_STM32SINK_HPP
#define COMPONENTS_STM32_STM32SINK_HPP

#include <vector>

#include "UartDevice.hpp"

class UartSink {
public:
	using Callback = void(*)(const void *, size_t);

	UartSink(int num, gpio_num_t rxPin, gpio_num_t txPin, int rate, uart_parity_t parity = UART_PARITY_DISABLE,
		uart_stop_bits_t stopBits = UART_STOP_BITS_1);
	void addCallback(Callback);
	void run(size_t timeoutUs);  // Listen

private:
	static constexpr const size_t kBufferSize = 64;
	UartDevice uart;
	std::vector<Callback> callbacks;
};

#endif  // COMPONENTS_STM32_STM32SINK_HPP
