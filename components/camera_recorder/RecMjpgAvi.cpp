//
// RecMjpgAvi.cpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/RecMjpgAvi.hpp"
#include "utility/time.hpp"

using namespace CameraRecorder;

void RecMjpgAvi::updateFps()
{
	if (stat.frames < kFrameRegCount) {
		++stat.frames;
	}
}

void RecMjpgAvi::calculateFps()
{
	if (std::isnan(stat.fps)) {
		float frames = static_cast<float>(stat.frames);
		std::chrono::duration<float> duration = std::chrono::microseconds(Utility::bootTimeUs()) - stat.started;
		stat.fps = frames / duration.count();
	}
}

void RecMjpgAvi::onNewFrame(Key::Type aFrame)
{
	if (stat.running) {
		frame = aFrame;
		stat.width = aFrame->width();
		stat.height = aFrame->height();
	}
}

bool RecMjpgAvi::start(const char *aFilename)
{
	if ((stat.fd = AVI_open_output_file(const_cast<char *>(aFilename)))) {
		stat.started = std::chrono::microseconds(Utility::bootTimeUs());
	    stat.frames  = 0;
	    stat.fps     = NAN;
		thread       = std::thread(&RecMjpgAvi::recordRoutine, this);
		stat.running = true;
		return true;
	}
	return false;
}

void RecMjpgAvi::stop()
{
	stat.running = false;
	if (thread.joinable()) {
		thread.join();
	}
}

void RecMjpgAvi::recordRoutine()
{
	decltype(frame) f;

	while (stat.running) {
		f = frame;  // Safely acquired ownership through incrementing smart pointer's counter
		if (f) {
			AVI_write_frame(stat.fd, reinterpret_cast<char *>(const_cast<void *>(f->data())), f->size());
			updateFps();
		}
	}

	// Set video properties
	calculateFps();
	AVI_set_video(stat.fd, stat.width, stat.height, stat.fps, const_cast<char *>("MJPG"));
}