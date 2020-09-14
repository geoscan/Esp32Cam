#ifndef WIFI2UARTBRIDGE_HPP
#define WIFI2UARTBRIDGE_HPP

#include <lwip/sockets.h>
#include <string.h>
#include <errno.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_event.h>
#include <esp_system.h>
#include <stdio.h>

#include "sdkconfig.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "UartDevice.hpp"
#include "TcpStream.hpp"

class Wifi2UartBridge {
public:
	Wifi2UartBridge():
		serial(UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, 2000000, UART_PARITY_DISABLE, UART_STOP_BITS_1),
		tcp{8001}
	{
//		bridge = this;
	}

	void start();

	void tcpWrite(const void *data, int size);
	size_t tcpRead(void *data, int maxSize);
	void serialWrite(const void *data, int size);
	size_t serialRead(void *data, int maxSize);
	size_t serialBytesToRead();

	bool anyConnections();

//	static Wifi2UartBridge *bridge;

	static SemaphoreHandle_t xSemaphore;
	static StaticSemaphore_t xSemaphoreBuffer;
private:
	static void socket_server_task(void *ignore);
	static void socket_server_listening_task(void *ignore);

	UartDevice serial;
	TcpStream tcp;
};

#endif // WIFI2UARTBRIDGE_HPP
