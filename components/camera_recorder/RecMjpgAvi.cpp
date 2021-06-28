//
// RecMjpgAvi.cpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_log.h>
#include "camera_recorder/RecMjpgAvi.hpp"
#include "utility/time.hpp"

using namespace CameraRecorder;

static constexpr const char *kTag = "[camera_recorder: mjpeg/avi]";

void RecMjpgAvi::logWriting(Key::Type frame)
{
	static constexpr std::size_t kPeriod = 50;
	static std::size_t iter = 0;

	if (iter ++ == kPeriod) {
		iter = 0;
		ESP_LOGI(kTag, "Record -- writing frame, %dx%d, %dkb",
			frame->width(), frame->height(), frame->size() / 1024);
	}
}

void RecMjpgAvi::updateFps()
{
	if (stat.frames < kFrameRegCount) {
		++stat.frames;
	} else if (stat.frames == kFrameRegCount) {
		calculateFps();
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

	if (aFrame.get() != nullptr && aFrame->data() != nullptr && aFrame->size()) {
		stat.width = aFrame->width();
		stat.height = aFrame->height();
		if (AVI_write_frame(stat.fd, reinterpret_cast<char *>(const_cast<void *>(aFrame->data())), aFrame->size()) != 0) {

			ESP_LOGW(kTag, "Recording -- failed to write a frame");

		} else {
			updateFps();
			logWriting(aFrame);
		}
	} else {

		ESP_LOGW(kTag, "Recording -- empty frame");

	}
}

bool RecMjpgAvi::start(const char *aFilename)
{
	if ((stat.fd = AVI_open_output_file(const_cast<char *>(aFilename))) != nullptr) {
		ESP_LOGI(kTag, "Record -- started: %s", aFilename);
		stat.started = std::chrono::microseconds(Utility::bootTimeUs());
	    stat.frames  = 0;
	    stat.fps     = NAN;
		key.enableSubscribe(true);
		return true;
	}
	return false;
}

void RecMjpgAvi::stop()
{
	key.enableSubscribe(false);

	calculateFps();
	if (stat.fd) {
		AVI_set_video(stat.fd, stat.width, stat.height, stat.fps, const_cast<char *>("MJPG"));
		AVI_close(stat.fd);
		ESP_LOGI(kTag, "Record -- stopped, frame: %dx%d, fps: %f", stat.width, stat.height, stat.fps);
	}

	stat.fd = nullptr;
}