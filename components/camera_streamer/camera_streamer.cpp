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

static void cameraStreamerTask(void *)
{
	static asio::io_context    context;
	static CameraStream        cameraStream(context, kSourceUdpPort);
	static CameraStreamControl cameraStreamControl(context, kSourceTcpPort, cameraStream);

	pthread_t stub;
	pthread_create(&stub, NULL, run<asio::io_context>, reinterpret_cast<void *>(&context));
	pthread_create(&stub, NULL, run<CameraStream>,     reinterpret_cast<void *>(&cameraStream));

	cameraStreamControl.asyncRun();
}

void cameraStreamerStart()
{
	// Create a process
	static const BaseType_t coreId = 0;
	xTaskCreatePinnedToCore(cameraStreamerTask, "CamStream", 4096, NULL, 1, NULL, coreId);
}