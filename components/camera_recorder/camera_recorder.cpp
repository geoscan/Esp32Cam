//
// camera_recorder.cpp
//
// Created on: Mar 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_CPP
#define CAMERA_RECORDER_CAMERA_RECORDER_CPP

#include "camera_recorder/Recorder.hpp"
#include "utility/Threading.hpp"

extern "C" void cameraRecorderInit()
{
	static CameraRecorder::Recorder recorder;

	Utility::Threading::setThreadCoreAffinity(0);
	static std::thread thread(&CameraRecorder::Recorder::operator(), &recorder);
}

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_CPP
