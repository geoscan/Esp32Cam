//
// camera_thread.cpp
//
// Created on: Apr 07, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "CameraStream.hpp"
#include "camera_thread/camera_thread.hpp"
#include "utility/thr/Threading.hpp"
#include <sdkconfig.h>

using namespace CameraThread;

static void cameraThreadTask(void *)
{
	static CameraStream cameraStream;
	cameraStream();
}

void cameraThreadInit()
{
	static TaskHandle_t taskHandle;
	xTaskCreatePinnedToCore(cameraThreadTask, "camera_thread", CONFIG_PTHREAD_STACK_MIN, 0,
		Ut::Thr::FreertosTask::PriorityLowest, &taskHandle, CONFIG_CAMERA_THREAD_PIN_TO_CORE);
}
