#ifndef WIFI2UARTBRIDGE_CPP
#define WIFI2UARTBRIDGE_CPP

#include "Wifi2UartBridge.hpp"
#include <esp_log.h>

//Wifi2UartBridge *Wifi2UartBridge::bridge = nullptr;
char tag[] = "Wifi2Uart";

void Wifi2UartBridge::socket_server_task(void *param)
{
	uint8_t rxBuffer[512];
	uint8_t txBuffer[512];
	auto bridge = reinterpret_cast<Wifi2UartBridge *>(param);
	while (true) {
		int len;
		int to_write;
		len = bridge->tcpRead(rxBuffer, sizeof(rxBuffer));
		if (len > 0) {
			bridge->serialWrite(rxBuffer, len);
		}
		to_write = bridge->serialBytesToRead();
		auto max = sizeof(txBuffer);
		bridge->serialRead(txBuffer, std::min(static_cast<uint32_t>(to_write), max));
		if (to_write > 0) {
			bridge->tcpWrite(txBuffer, to_write);
		}
	}
}

void Wifi2UartBridge::start()
{
	ESP_LOGE(tag, "Start");
	tcp.start();
	sleep(1);
	xTaskCreate(Wifi2UartBridge::socket_server_task, "wifi_to_uart_bridge", 5120, this, 5, NULL);
}

bool Wifi2UartBridge::anyConnections()
{
		return tcp.anyClientSockets();
}

void Wifi2UartBridge::tcpWrite(const void *data, int size)
{
	auto rest = static_cast<uint32_t>(size);
	while (rest > 0) {
		int written = tcp.write(static_cast<const uint8_t *>(data) + (size - rest), static_cast<size_t>(rest));
		rest -= written;
	}
}

size_t Wifi2UartBridge::tcpRead(void *data, int maxSize)
{
	return tcp.read(data, static_cast<size_t>(maxSize));
}

void Wifi2UartBridge::serialWrite(const void *data, int size)
{
	serial.write(data, static_cast<size_t>(size));
}

size_t Wifi2UartBridge::serialRead(void *data, int maxSize)
{
	if (maxSize < 1) {
		return 0;
	}
	return serial.read(data, static_cast<size_t>(maxSize));
}

size_t Wifi2UartBridge::serialBytesToRead()
{
	return serial.bytesToRead();
}

#endif // WIFI2UARTBRIDGE_CPP
