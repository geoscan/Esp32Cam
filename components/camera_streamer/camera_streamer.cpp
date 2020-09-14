#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "CameraStreamer.hpp"

void cameraStreamerTask(void *)
{
	static const uint16_t kSinkPort   = 10001;
	static const uint16_t kSourcePort = 10000;

	asio::io_context context;
	CameraStreamer   cameraStreamer(context, kSourcePort, kSinkPort);

	cameraStreamer.run();
}

void cameraStreamerStart()
{
	static const BaseType_t coreId = 0;
	xTaskCreatePinnedToCore(cameraStreamerTask, "CamStream", 4096, NULL, 1, NULL, coreId);
}