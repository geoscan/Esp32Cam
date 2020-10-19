//
// main.cpp
//
// Created on: Aug 12, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <pthread.h>
#include <asio.hpp>
#include "wifi.h"
#include "http.h"
#include "rtsp.h"
#include "wifi_uart_bridge.hpp"
#include "deprecated_wifi_uart_bridge.hpp"
#include "camera_streamer.h"
#include "ov2640.hpp"
#include "version.hpp"

extern "C" int app_main(void)
{
	versionInit();
	wifiStart();
	httpStart();
	ov2640Init();

	static asio::io_context context(3);
	cameraStreamerStart(context);
	wifiUartBridgeStart(context);

	return 0;
}
