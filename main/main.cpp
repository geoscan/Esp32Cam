//
// main.cpp
//
// Created on: Aug 12, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include "wifi.h"
#include "http.h"
#include "rtsp.h"
#include "echo_tcp.hpp"
#include "wifi_uart_bridge.hpp"

extern "C" int app_main(void)
{
//	esp_timer_init();
	wifiStart();
//	rtspStart();
	httpStart();
	wifiUartBridgeStart();

	return 0;
}
