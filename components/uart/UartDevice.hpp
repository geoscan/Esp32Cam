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
#include "utility/Buffer.hpp"

//!
//! \brief The UartDevice class
//!
class UartDevice {
public:
	UartDevice(int num,  gpio_num_t rxPin, gpio_num_t txPin, int rate, uart_parity_t parity = UART_PARITY_DISABLE,
			   uart_stop_bits_t stopBits = UART_STOP_BITS_1);
	virtual ~UartDevice();

	///
	/// \brief  Read from UART interface associated with this RAII UART wrapper
	/// \param  aWaitMs Block duration. 0 for persistent block
	/// \return Number of bytes read
	///
	size_t read(void *data, size_t size, size_t aWaitMs = 0);
	size_t read(Utility::Buffer, size_t aWaitMs = 0);

	///
	/// \return Number of bytes written
	///
	size_t write(const void *data, size_t size);
	size_t write(Utility::ConstBuffer buffer);
	size_t bytesToRead();

	int getNum() const;
private:
	static constexpr size_t kBufferSize{1024};

	int uartNum;
	int currentRate;
};

#endif  // PLATFORM_LINUX_DRIVERS_UARTDEVICE_HPP_
