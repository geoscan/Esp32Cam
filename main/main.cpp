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
#include "camera_streamer.h"
#include "cam/cam.hpp"
#include "camera_thread/camera_thread.hpp"
#include "camera_recorder/camera_recorder.hpp"
#include "sd_fat.h"
#include "log_udp.h"
#include "mav/mav.hpp"
#include "uart/uart.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include "socket/socket.hpp"
#include "wq/wq.hpp"
#include "tracking/tracking.hpp"
#include "module/module.hpp"
#include "utility/time.hpp"
#include <nvs_flash.h>

static asio::io_context context(3);

extern "C" int app_main(void)
{
	Ut::Tim::taskDelay(std::chrono::milliseconds(1000));  // Time window to connect a hardware debugger, before JTAG pins are reconfigured
	nvs_flash_init();
	Wq::start();
	CameraRecorder::init();
	camInit();
	esp_event_loop_create_default();
	Mod::init();
	wifiStart();
	httpStart();
#if !CONFIG_DRIVER_OV2640_USE_HOOKS
	cameraThreadInit();
#endif
	cameraStreamerStart(context);
	Mav::init();
	Bdg::init();
	Sock::start();
	Uart::start();
	Trk::init();
	return 0;
}
