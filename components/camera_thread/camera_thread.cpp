//
// camera_thread.cpp
//
// Created on: Apr 07, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "CameraStream.hpp"
#include "camera_thread/camera_thread.hpp"
#include "utility/Threading.hpp"

using namespace CameraThread;

void cameraThreadInit()
{
		static CameraStream cameraStream;

		Utility::Threading::setThreadCoreAffinity(1);
		static std::thread threadCameraStream(&CameraStream::operator(), &cameraStream);
}