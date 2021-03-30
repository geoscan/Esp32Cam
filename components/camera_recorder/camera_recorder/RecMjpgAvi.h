//
// RecMjpgAvi.h
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H
#define CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H

#include <type_traits>
#include <chrono>
#include "Record.hpp"

extern "C" {
#include "avilib/avilib.h"
}

namespace CameraRecorder {

class RecMjpgAvi : public Record {
private:
	std::thread thread;
	std::decay<Key::Type>::type frame;

	static constexpr std::size_t kFrameRegCount = 200;
	struct {
		avi_t *fd;
		bool running = false;
		std::chrono::microseconds started;
		std::size_t frames = 0;
		float fps;
	} stat;

	void updateFps();
	void calculateFps();
	void onNewFrame(Key::Type) override;
	void recordRoutine();
public:
	using Record::Record;
	bool start(const char *filename) override;
	void stop() override;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_RECMJPGAVI_H
