//
// wifi_uart_bridge.cpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Wifi2UartBridge.hpp"
#include "deprecated_wifi_uart_bridge.hpp"

void deprecatedWifiUartBridgeStart()
{
	static Wifi2UartBridge bridge;
	bridge.start();
//	vTaskSuspend(NULL);

}