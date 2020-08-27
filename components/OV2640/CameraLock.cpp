#include "CameraLock.hpp"

using namespace CamOv2640;

asio::detail::mutex CameraLock::cameraMutex;

CameraLock::CameraLock()
{
	cameraMutex.lock();
}

CameraLock::~CameraLock()
{
	cameraMutex.unlock();
}