//
// RecMjpgAvi.h
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H
#define CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H

#include "Record.hpp"
#include "utility/thr/Semaphore.hpp"
#include "sub/Cam.hpp"
#include "utility/mod/ModuleBase.hpp"
#include <type_traits>
#include <chrono>
#include <cmath>

extern "C" {
#include "avilib/avilib.h"
}

namespace CameraRecorder {

class RecMjpgAvi : public Record, public Utility::Mod::ModuleBase {
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

	struct {
		Sub::Cam::RecordStart recordStart;
		Sub::Cam::RecordStop recordStop;
	} sub;

private:
	void updateFps();
	void calculateFps();
	void onNewFrame(Key::Type) override;
	void logWriting(Key::Type);
	bool startWrap(const char *filename);
public:
	RecMjpgAvi();
	void getFieldValue(Utility::Mod::Fld::Req, Utility::Mod::Fld::OnResponseCallback) override;
	bool start(const char *filename) override;
	void stop() override;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H
