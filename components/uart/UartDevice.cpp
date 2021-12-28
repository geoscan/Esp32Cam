#include <cassert>
#include <cstring>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "UartDevice.hpp"

UartDevice::UartDevice(int num, gpio_num_t rxPin, gpio_num_t txPin, int rate, uart_parity_t parity,
					   uart_stop_bits_t stopBits):
	uartNum{num},
	currentRate{rate}
{
	uart_config_t uart_config = {
		.baud_rate = rate,
		.data_bits = UART_DATA_8_BITS,
		.parity = parity,
		.stop_bits = stopBits,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
		.source_clk = UART_SCLK_APB,
	};
	// We won't use a buffer for sending data.
	if (!uart_is_driver_installed(num)) {
		uart_set_pin(num, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		uart_driver_install(num, kBufferSize * 2, kBufferSize * 2, 0, NULL, 0);
	}
	uart_param_config(num, &uart_config);
}

UartDevice::~UartDevice()
{
}

size_t UartDevice::read(void *data, size_t size, size_t aWaitMs)
{
	ssize_t count = uart_read_bytes(uartNum, static_cast<uint8_t *>(data), size,
		aWaitMs ? aWaitMs / portTICK_RATE_MS : 0);
	return count > 0 ? static_cast<uint32_t>(count) : 0;
}

size_t UartDevice::write(const void *data, size_t size)
{
	ssize_t count = uart_write_bytes(uartNum, static_cast<const char *>(data), size);
	return count > 0 ? static_cast<size_t>(count) : 0;
}

size_t UartDevice::bytesToRead()
{
	size_t bytesAvailable;

	if (uart_get_buffered_data_len(uartNum, &bytesAvailable) >= 0)
		return static_cast<size_t>(bytesAvailable);
	else
		return 0;
}

int UartDevice::getNum() const
{
	return uartNum;
}
