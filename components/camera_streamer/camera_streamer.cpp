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

void cameraStreamerStart()
{
	asio::io_context    controlContext;
	asio::io_context    streamContext;
	CameraStream        cameraStream(streamContext, 8887);
	CameraStreamControl cameraStreamControl(controlContext, 8888, cameraStream);
//	auto cameraStreamControl(std::make_shared<CameraStreamControl>(controlContext, 8888, cameraStream));

	pthread_t stub;
	pthread_create(&stub, NULL, run<asio::io_context>,   reinterpret_cast<void *>(&streamContext));
	pthread_create(&stub, NULL, run<asio::io_context>,   reinterpret_cast<void *>(&controlContext));
	pthread_create(&stub, NULL, run<CameraStream>,       reinterpret_cast<void *>(&cameraStream));
//	pthread_create(&stub, NULL, run<CameraStreamControl, reinterpret_cast<void *>(&cameraStreamControl));

	cameraStreamControl.run();
}

//void cameraStreamerStart()
//{
////	xTaskCreatePinnedToCore(cameraStreamerTask, "task_camera_stream", 1024, 0, tskIDLE_PRIORITY, NULL, 0);
//	xTaskCreatePinnedToCore(cameraStreamerTask, "camst", 4096, 0, 5, 0, 1);
//	vTaskSuspend(NULL);
//}