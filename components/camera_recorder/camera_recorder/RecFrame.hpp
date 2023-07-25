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
#include "utility/thr/Semaphore.hpp"
#include "utility/MakeSingleton.hpp"

namespace CameraRecorder {

class RecFrame : public Record, public Ut::MakeSingleton<RecFrame> {
private:
	FILE *file;
	struct {
		Ut::Thr::Semaphore<1, 0> sem;
		std::mutex mut;
	} sync;
protected:
	void onNewFrame(Key::Type) override;
	static constexpr std::chrono::seconds kFrameTimeout{1};
public:
	using CameraRecorder::Record::Record;

	bool start(const char *) override;
	void stop() override;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_RECFRAME_HPP
