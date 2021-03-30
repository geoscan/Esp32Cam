//
// RecMjpgAvi.cpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/RecMjpgAvi.h"
#include "utility/time.hpp"

using namespace CameraRecorder;

void RecMjpgAvi::updateFps()
{

}

void RecMjpgAvi::onNewFrame(Key::Type aFrame)
{
	frame = aFrame;
	updateFps();
}

bool RecMjpgAvi::start(const char *aFilename)
{
	if (stat.fd = AVI_open_output_file(const_cast<char *>(aFilename))) {
		stat.started = std::chrono::microseconds(Utility::bootTimeUs());
	    stat.running = true;
	    stat.frames  = 0;
		thread       = std::thread(&RecMjpgAvi::recordRoutine, this);
		return true;
	}
	return false;
}

void RecMjpgAvi::stop()
{
	stat.running = false;
	thread.join();
}

