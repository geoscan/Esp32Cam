//
// RecFrame.hpp
//
// Created on: Apr 05, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_RECFRAME_HPP
#define CAMERA_RECORDER_CAMERA_RECORDER_RECFRAME_HPP

#include <chrono>
#include "Record.hpp"
#include "utility/Semaphore.hpp"
#include "sub/Cam.hpp"

namespace CameraRecorder {

class RecFrame : public Record {
private:
	FILE *file;
	struct {
		Utility::Semaphore<1, 0> sem;
		std::mutex mut;
	} sync;

	struct {
		Sub::Cam::Shot shot;
	} keys;

protected:
	void onNewFrame(Key::Type) override;
	static constexpr std::chrono::seconds kFrameTimeout{1};

public:
	RecFrame();

	bool start(const char *) override;
	void stop() override;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_RECFRAME_HPP
