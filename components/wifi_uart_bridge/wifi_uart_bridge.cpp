#include <pthread.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#include "wifi_uart_bridge.hpp"
#include "utility/Run.hpp"
#include "Bridge.hpp"
#include "UartEndpoint.hpp"
#include "UdpEndpoint.hpp"

void wifiUartBridgeStart()
{
	static pthread_t        pth;
	static UartEndpoint     uart(UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, 2000000, UART_PARITY_DISABLE, UART_STOP_BITS_1);
	static asio::io_context context;
	static UdpEndpoint      udp(context);
	static Bridge           bridge(uart, udp);

	pthread_create(&pth, NULL, Utility::run<Bridge>, &bridge);
	pthread_create(&pth, NULL, Utility::run<asio::io_context>, &context);
}