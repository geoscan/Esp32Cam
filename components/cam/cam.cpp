//
// cam.cpp
//
// Created on: Mar 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sdkconfig.h"
#include "cam/cam.hpp"
#include "cam/Camera.hpp"
#include "Ov2640.hpp"

template <typename T>
static void init()
{
	static T cam;
	cam.init();
	Cam::Camera::setInstance(cam);
}

void camInit()
{
#ifdef CONFIG_CAM_MODEL_OV2640
	init<Ov2640>();
#elif
#endif // CONFIG_CAM_MODEL_OV2640
}