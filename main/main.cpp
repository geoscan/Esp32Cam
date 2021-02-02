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
#include "Ov2640.hpp"
#include "version.hpp"

static asio::io_context context(3);

extern "C" void sdInit();

extern "C" int app_main(void)
{
	sdInit();
	versionInit();
	wifiStart();
	httpStart();
	ov2640Init();

	cameraStreamerStart(context);
	wifiUartBridgeStart(context);

	return 0;
}
