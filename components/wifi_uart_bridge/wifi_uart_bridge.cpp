#include "Wifi2UartBridge.hpp"
#include "wifi_uart_bridge.hpp"

void wifiUartBridgeStart()
{
	Wifi2UartBridge bridge;
	bridge.start();
	vTaskSuspend(NULL);
}