#ifndef COMPONENTS_RTSP_CAMERALOCK_HPP
#define COMPONENTS_RTSP_CAMERALOCK_HPP

#include "asio.hpp"
#include "freertos/semphr.h"

namespace CamOv2640 {

/// RAII object, locks access to the camera
class CameraLock final {
	static SemaphoreHandle_t semaphore;
public:
	CameraLock();
	~CameraLock();

	CameraLock(const CameraLock &) = delete;
	CameraLock(CameraLock &&) = delete;
	CameraLock &operator=(const CameraLock &) = delete;
	CameraLock &operator=(CameraLock &&) = delete;
};

}

#endif // COMPONENTS_RTSP_CAMERALOCK_HPP
