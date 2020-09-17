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
void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

#include "Ov2640.hpp"
extern "C" void cameraStreamerTask(void *)
{
	asio::io_context    context;
	CameraStream        cameraStream(context, kSourceUdpPort);
//	CameraStreamControl cameraStreamControl(context, kSourceTcpPort, cameraStream);
	auto cameraStreamControl(std::make_shared<CameraStreamControl>(context, kSourceTcpPort, cameraStream));

	pthread_t stub;
	pthread_create(&stub, NULL, run<asio::io_context>, reinterpret_cast<void *>(&context));
	pthread_create(&stub, NULL, run<CameraStream>,     reinterpret_cast<void *>(&cameraStream));

//	cameraStreamControl.asyncRun();
	cameraStreamControl->asyncRun();

	while(1) {}
}

void cameraStreamerStart()
{
//	xTaskCreatePinnedToCore(cameraStreamerTask, "task_camera_stream", 1024, 0, tskIDLE_PRIORITY, NULL, 0);
	xTaskCreatePinnedToCore(cameraStreamerTask, "camst", 4096, 0, 5, 0, 1);
	vTaskSuspend(NULL);
}