//
// Camera.cpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "cam/Camera.hpp"

using namespace Cam;

void Camera::setInstance(CameraBase &aCameraBase)
{
	instance = &aCameraBase;
}

CameraBase &Camera::getInstance()
{
	if (!instance) {
		static CameraBase base;
		return base;
	}
	return *instance;
}

