//
// Camera.hpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAM_CAM_CAMERA_HPP
#define CAM_CAM_CAMERA_HPP

#include <memory>
#include "utility/cont/Buffer.hpp"
#include "Frame.hpp"

namespace Cam {

class CameraBase {
public:
	virtual std::shared_ptr<Frame> getFrame()
	{
		return std::make_shared<Frame>();
	}

	virtual ~CameraBase() = default;

	virtual void init()
	{
	}
};

class Camera {
private:
	static CameraBase *instance;
public:
	static void setInstance(CameraBase &aCameraBase);
	static CameraBase &getInstance();
};

}  // namespace Cam

#endif  // CAM_CAM_CAMERA_HPP
