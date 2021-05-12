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
#include "wifi_uart_bridge.hpp"
#include "deprecated_wifi_uart_bridge.hpp"
#include "camera_streamer.h"
#include "cam/cam.hpp"
#include "camera_thread/camera_thread.hpp"
#include "version.hpp"
#include "sd_fat.h"
#include "log_udp.h"

static asio::io_context context(3);

extern "C" int app_main(void)
{
	camInit();
	versionInit();

	esp_event_loop_create_default();
	wifiStart();
	httpStart();
	cameraThreadInit();

	logUdpStart(context);
	cameraStreamerStart(context);
	wifiUartBridgeStart(context);
	return 0;
}
