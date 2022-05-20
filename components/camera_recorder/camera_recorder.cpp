//
// camera_recorder.cpp
//
// Created on: May 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/camera_recorder.hpp"
#include "camera_recorder/Storage.hpp"
#include <esp_log.h>

namespace CameraRecorder {

const char *kDebugTag = TARGET_DEBUG_TAG;

void init()
{
	static Storage storage{};
	(void)storage;
}

}  // namespace CameraRecorder
