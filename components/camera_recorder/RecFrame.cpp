//
// RecFrame.cpp
//
// Created on: Apr 05, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/RecFrame.hpp"

static constexpr const char *kTag = "[camera_recorder: frame]";

using namespace CameraRecorder;

void RecFrame::onNewFrame(Key::Type frame)
{
	std::lock_guard<std::mutex> lock(sync.mut);

	if (file) {
		fwrite(frame->data(), 1, frame->size(), file);
		fclose(file);
		file = nullptr;
	}
	sync.sem.release();  // Notify that frame's been written
}

bool RecFrame::start(const char *aFilename)
{
	{
		std::lock_guard<std::mutex> lock(sync.mut);
		file = fopen(aFilename, "wb");
		if (!file) {
			ESP_LOGW(kTag, "Photo -- unable to open the file %s", aFilename);
			return false;
		}
	}

	// First try to acquire an image from the frame stream
	key.enableSubscribe(true);
	sync.sem.try_acquire_for(kFrameTimeout);
	key.enableSubscribe(false);
	sync.sem.try_acquire();  // Reset

	std::lock_guard<std::mutex> lock(sync.mut);
	bool ret = (file == nullptr);

	if (file) {  // There was no frame stream in place, try "manually"
		auto frame = Cam::Camera::getInstance().getFrame();
		if (frame && frame->valid() && fwrite(frame->data(), 1, frame->size(), file) == frame->size()) {
			ret = true;
		}
		fclose(file);
		file = nullptr;
	}

	if (ret) {
		ESP_LOGI(kTag, "Photo -- successfully wrote frame file %s", aFilename);
	} else {
		ESP_LOGW(kTag, "Photo -- failed to acquire a frame");
	}

	return ret;
}

void RecFrame::stop()
{
}