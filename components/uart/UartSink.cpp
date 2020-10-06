//
// Stm32Sink.hpp
//
// Created on:  Oct 05, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "UartSink.hpp"
#include "utility/time.hpp"

UartSink::UartSink(int num,  gpio_num_t rxPin, gpio_num_t txPin, int rate, uart_parity_t parity, uart_stop_bits_t stopBits) :
	uart(num, rxPin, txPin, rate, parity, stopBits)
{
}

void UartSink::addCallback(Callback cb)
{
	callbacks.push_back(cb);
}

void UartSink::run(size_t timeoutUs)
{
	auto timeStartUs = Utility::bootTimeUs();
	char buf[kBufferSize];

	do {
		size_t nread = uart.read(buf, kBufferSize);

		if (nread > 0) {
			for (Callback &callback : callbacks) {
				callback(buf, nread);
			}
		}
	} while (!Utility::expired(timeStartUs, timeoutUs));
}