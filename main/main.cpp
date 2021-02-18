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
#include <esp_event.h>
#include "wifi.h"
#include "http.h"
#include "rtsp.h"
#include "wifi_uart_bridge.hpp"
#include "deprecated_wifi_uart_bridge.hpp"
#include "camera_streamer.h"
#include "Ov2640.hpp"
#include "version.hpp"
#include "sd_fat.h"

static asio::io_context context(3);

extern "C" int app_main(void)
{
	esp_event_loop_create_default();
	versionInit();
	wifiStart();
	httpStart();
	ov2640Init();
	sdFatInit();

	cameraStreamerStart(context);
	wifiUartBridgeStart(context);

	return 0;
}
