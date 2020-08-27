#ifndef COMPONENTS_RTSP_CAMERALOCK_HPP
#define COMPONENTS_RTSP_CAMERALOCK_HPP

#include "asio.hpp"

namespace CamOv2640 {

/// RAII object, locks access to the camera
class CameraLock final {
	static asio::detail::mutex cameraMutex;
public:
	CameraLock();
	~CameraLock();
};

}

#endif // COMPONENTS_RTSP_CAMERALOCK_HPP
