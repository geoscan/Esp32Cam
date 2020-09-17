#include "CameraLock.hpp"

using namespace CamOv2640;

//asio::detail::mutex CameraLock::cameraMutex;
SemaphoreHandle_t CameraLock::semaphore = xSemaphoreCreateMutex();

CameraLock::CameraLock()
{
//	cameraMutex.lock();
	while (xSemaphoreTake(semaphore, portMAX_DELAY) != pdTRUE);  // No loop body
}

CameraLock::~CameraLock()
{
	xSemaphoreGive(semaphore);
}