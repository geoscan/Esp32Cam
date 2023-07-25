//
// main.cpp
//
// Created on: Aug 12, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "cam/cam.hpp"
#include "camera_recorder/camera_recorder.hpp"
#include "camera_streamer.h"
#include "camera_thread/camera_thread.hpp"
#include "http.h"
#include "log_udp.h"
#include "mav/mav.hpp"
#include "module/module.hpp"
#include "sd_fat.h"
#include "socket/socket.hpp"
#include "tracking/tracking.hpp"
#include "uart/uart.hpp"
#include "utility/time.hpp"
#include "wifi.h"
#include "wifi_uart_bridge.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include "wq/wq.hpp"
#include <asio.hpp>
#include <esp_event.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <pthread.h>

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
	Bft::init();
	return 0;
}
