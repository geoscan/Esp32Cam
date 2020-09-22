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

template <typename Runnable>
static void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

void cameraStreamerStart()
{
	static asio::io_context    controlContext;
	static asio::io_context    streamContext;
	static CameraStream        cameraStream(streamContext, CONFIG_CAMSTREAM_SOURCE_UDP_PORT);
	static CameraStreamControl cameraStreamControl(controlContext, CONFIG_CAMSTREAM_SOURCE_TCP_PORT, cameraStream);
//	auto cameraStreamControl(std::make_shared<CameraStreamControl>(controlContext, 8888, cameraStream));

	static pthread_t stub;
	pthread_create(&stub, NULL, run<asio::io_context>,    reinterpret_cast<void *>(&streamContext));
	pthread_create(&stub, NULL, run<asio::io_context>,    reinterpret_cast<void *>(&controlContext));
	pthread_create(&stub, NULL, run<CameraStream>,        reinterpret_cast<void *>(&cameraStream));
	pthread_create(&stub, NULL, run<CameraStreamControl>, reinterpret_cast<void *>(&cameraStreamControl));

//	cameraStreamControl.run();
}