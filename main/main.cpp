//
// main.cpp
//
// Created on: Aug 12, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include "wifi.h"
#include "http.h"
#include "rtsp.h"
#include "echo_tcp.hpp"
#include "wifi_uart_bridge.hpp"
#include "deprecated_wifi_uart_bridge.hpp"
#include "camera_streamer.h"

extern "C" int app_main(void)
{
	wifiStart();
	httpStart();
//	cameraStreamerStart();
	wifiUartBridgeStart();
//	deprecatedWifiUartBridgeStart();
	return 0;
}
