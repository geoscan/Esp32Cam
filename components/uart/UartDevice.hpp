/*
 * UartDevice.hpp
 *
 *  Created on: Feb 28, 2014
 *      Author: Alexander
 */

#ifndef PLATFORM_LINUX_DRIVERS_UARTDEVICE_HPP_
#define PLATFORM_LINUX_DRIVERS_UARTDEVICE_HPP_

#include "driver/uart.h"
#include "driver/gpio.h"
//#include "SerialDevice.hpp"

//!
//! \brief The UartDevice class
//!
class UartDevice {
public:
	UartDevice(int num,  gpio_num_t rxPin, gpio_num_t txPin, int rate, uart_parity_t parity = UART_PARITY_DISABLE,
			   uart_stop_bits_t stopBits = UART_STOP_BITS_1);
	//!
	//! \brief UartDevice
	//! \param num
	//! \param rate
	//! \param blocking or non-blocking write
	//! \param parity
	//! \param stopBits
	//!
	virtual ~UartDevice();

	size_t read(void *data, size_t size);
	size_t write(const void *data, size_t size);
	size_t bytesToRead();
private:
	static constexpr size_t kBufferSize{1024};

	int uartNum;
	int currentRate;
};

#endif  // PLATFORM_LINUX_DRIVERS_UARTDEVICE_HPP_
