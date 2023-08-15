//
// RecFrame.cpp
//
// Created on: Apr 05, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_log.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_CAMERA_RECORDER_DEBUG_LEVEL)
#include "camera_recorder/RecFrame.hpp"
#include "module/ModuleBase.hpp"
#include "camera_recorder/camera_recorder.hpp"

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
	if (aFilename == nullptr) {
		ESP_LOGE(kTag, "failed to start recording, as no file name has been provided");

		return false;
	}

	// Save frame in a higher resolution.
	// Save the currently used camera resolution
	constexpr int kUninitialized = 0;
	constexpr std::pair<int, int> kFrameSizePhoto{800, 600};  // TODO: rewrite w/ parameters
	std::pair<int, int> frameSizePrev{kUninitialized, kUninitialized};
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[&frameSizePrev](std::pair<int, int> aFrameSize) {frameSizePrev = aFrameSize; });

	if (frameSizePrev.first == 0) {  // The camera module has not produced a response
		ESP_LOGW(kTag, "start -- fail (camera's current frame size acquisition)");

		return false;
	}

	// Open the file to save the frame into
	{
		std::lock_guard<std::mutex> lock(sync.mut);
		file = fopen(aFilename, "wb");
		ESP_LOGD(kDebugTag, "start. opening file \"%s\"", aFilename);

		if (!file) {
			ESP_LOGW(kDebugTag, "start. Photo -- unable to open the file %s. Capture is aborted", aFilename);

			return false;
		}
	}
	// Switch the camera to a higher resolution
	{
		ESP_LOGD(kDebugTag, "start. Initializing frame size %dx%d", kFrameSizePhoto.first, kFrameSizePhoto.second);
		bool frameSizeSetIsOk = false;
		Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
			kFrameSizePhoto,
			[&frameSizeSetIsOk](Mod::Fld::WriteResp aWriteResp)
			{
				frameSizeSetIsOk = aWriteResp.isOk();

				if (!aWriteResp.isOk()) {
					ESP_LOGW(kTag, "start -- fail (frame size setting)");
				}
			});

		if (!frameSizeSetIsOk) {
			ESP_LOGW(kDebugTag, "start. Frame size initialization failed");
			return false;
		}
		ESP_LOGI(kDebugTag, "start. Switched frame size to %dx%d", kFrameSizePhoto.first, kFrameSizePhoto.second);
	}
	// First try to acquire an image from the frame stream
	ESP_LOGD(kDebugTag, "start. Enabling frame subcription, acquiring a frame");
	key.enableSubscribe(true);
	sync.sem.try_acquire_for(kFrameTimeout);
	key.enableSubscribe(false);
	sync.sem.try_acquire();  // Reset
	// Write the acquired frame into file
	std::lock_guard<std::mutex> lock(sync.mut);
	bool ret = (file == nullptr);

	if (file) {  // There was no frame stream in place, try "manually"
		auto frame = Cam::Camera::getInstance().getFrame();
		ESP_LOGD(kDebugTag, "start. Writing to file \"%s\"", aFilename);
		if (frame && frame->valid() && fwrite(frame->data(), 1, frame->size(), file) == frame->size()) {
			ret = true;
		}
		fclose(file);
		file = nullptr;
	}

	if (ret) {
		ESP_LOGI(kTag, "Photo -- successfully wrote frame file %s", aFilename);
	} else {
		ESP_LOGW(kTag, "Photo -- writing to file \"%s\" failed", aFilename);
	}

	// Restore previous frame size
	ESP_LOGI(kDebugTag, "start. restoring previous frame size %dx%d", frameSizePrev.first, frameSizePrev.second);
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		frameSizePrev, [](Mod::Fld::WriteResp){});

	return ret;
}

void RecFrame::stop()
{
}
