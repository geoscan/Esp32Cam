#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "camera_streamer.h"
#include "CameraStreamer.hpp"

void cameraStreamerTask(void *)
{
	asio::io_context context;
	CameraStreamer   cameraStreamer(context, kSourcePort, kSinkPort, kFps);

	cameraStreamer.run();
}

void cameraStreamerStart()
{
	static const BaseType_t coreId = 0;
	xTaskCreatePinnedToCore(cameraStreamerTask, "CamStream", 4096, NULL, 1, NULL, coreId);
}