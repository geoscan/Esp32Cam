//
// camera_streamer.cpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <pthread.h>
#include <memory>

#include "camera_streamer.h"
#include "CameraStream.hpp"
#include "CameraStreamControl.hpp"

#ifndef CONFIG_CAMSTREAM_FPS
# define CONFIG_CAMSTREAM_FPS -1
#endif  // CONFIG_CAMSTREAM_FPS

template <typename Runnable>
static void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

void cameraStreamerStart(asio::io_context &context)
{
	static CameraStream        cameraStream(context, CONFIG_CAMSTREAM_SOURCE_UDP_PORT, CONFIG_CAMSTREAM_FPS);
	static CameraStreamControl cameraStreamControl(context, CONFIG_CAMSTREAM_SOURCE_TCP_PORT, cameraStream);

	static pthread_t stub;
	pthread_create(&stub, NULL, run<CameraStream>,        reinterpret_cast<void *>(&cameraStream));
	pthread_create(&stub, NULL, run<CameraStreamControl>, reinterpret_cast<void *>(&cameraStreamControl));
}