//
// RecMjpgAvi.cpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_log.h>
#include "camera_recorder/RecMjpgAvi.hpp"
#include "utility/time.hpp"
#include <sdkconfig.h>

using namespace CameraRecorder;

static constexpr const char *kTag = "[camera_recorder: mjpeg/avi]";

void RecMjpgAvi::logWriting(Sub::Key::NewFrameEvent frame)
{
	static constexpr std::size_t kPeriod = 50;
	static std::size_t iter = 0;

	if (iter ++ == kPeriod) {
		iter = 0;
		ESP_LOGI(kTag, "Record -- writing frame, %dx%d, %dkb",
			frame->width(), frame->height(), frame->size() / 1024);
	}
}

bool RecMjpgAvi::startWrap(const char *filename)
{
	std::string name;
	name.reserve(strlen(CONFIG_SD_FAT_MOUNT_POINT) + strlen(filename) + strlen(".avi") + 1);
	name.append(CONFIG_SD_FAT_MOUNT_POINT);
	name.append("/");
	name.append(filename);
	name.append(".avi");

	return start(name.c_str());
}

RecMjpgAvi::RecMjpgAvi() :
	Mod::ModuleBase(Mod::Module::Camera),
	sub{{&RecMjpgAvi::startWrap, this}, {&RecMjpgAvi::stop, this}}
{
	ESP_LOGI(kTag, "RecMjpgAvi initialized");
}

void RecMjpgAvi::getFieldValue(Mod::Fld::Req aReq, Mod::Fld::OnResponseCallback aOnResponse)
{
	switch (aReq.field) {
		case Mod::Fld::Field::Recording:
			aOnResponse(makeResponse<Mod::Module::Camera, Mod::Fld::Field::Recording>(
				nullptr != stat.fd));

			break;

		default:
			break;
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
		std::chrono::duration<float> duration = std::chrono::microseconds(Ut::bootTimeUs()) - stat.started;
		stat.fps = frames / duration.count();
	}
}

void RecMjpgAvi::onNewFrame(Sub::Key::NewFrameEvent aFrame)
{
	if (aFrame.get() != nullptr && aFrame->data() != nullptr && aFrame->size()) {
		static constexpr int kNoAviError = 0;
		stat.width = aFrame->width();
		stat.height = aFrame->height();
		int errorCode = AVI_write_frame(stat.fd, reinterpret_cast<char *>(const_cast<void *>(aFrame->data())), aFrame->size());

		if (errorCode != kNoAviError) {
			ESP_LOGW(kTag, "Recording -- failed to write a frame, error=%d(%s)", errorCode, AVI_strerror());
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
	// Process the "already started" case
	if (nullptr != stat.fd) {
		ESP_LOGW(kTag, "RecMjpgAvi - record has already been started");
		return false;
	}

	if ((stat.fd = AVI_open_output_file(const_cast<char *>(aFilename))) != nullptr) {
		ESP_LOGI(kTag, "Record -- started: %s", aFilename);
		stat.started = std::chrono::microseconds(Ut::bootTimeUs());
	    stat.frames  = 0;
	    stat.fps     = NAN;
		Record::key.setEnabled(true);
		return true;
	} else {
		ESP_LOGE(kTag, "Record - failed. Unable to open output file %s", aFilename);
	}
	return false;
}

void RecMjpgAvi::stop()
{
	Record::key.setEnabled(false);

	calculateFps();
	if (stat.fd) {
		AVI_set_video(stat.fd, stat.width, stat.height, stat.fps, const_cast<char *>("MJPG"));
		AVI_close(stat.fd);
		ESP_LOGI(kTag, "Record -- stopped, frame: %dx%d, fps: %f", stat.width, stat.height, stat.fps);
	}

	stat.fd = nullptr;
}
