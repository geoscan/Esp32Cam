//
// RecMjpgAvi.h
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H
#define CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H

#include "Record.hpp"
#include "utility/Semaphore.hpp"
#include "sub/Sys.hpp"
#include <type_traits>
#include <chrono>
#include <cmath>

extern "C" {
#include "avilib/avilib.h"
}

namespace CameraRecorder {

class RecMjpgAvi : public Record, public Sub::Sys::ModuleBase {
private:
	static constexpr std::size_t kFrameRegCount = 200;
	struct {
		avi_t *fd;
		std::chrono::microseconds started;

		std::size_t frames = 0;
		int width          = -1;
		int height         = -1;
		float fps          = NAN;
	} stat;

	void updateFps();
	void calculateFps();
	void onNewFrame(Key::Type) override;
	void logWriting(Key::Type);
public:
	RecMjpgAvi();
	void getFieldValue(Sub::Sys::Fld::Req, Sub::Sys::Fld::OnResponseCallback) override;
	bool start(const char *filename) override;
	void stop() override;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H
