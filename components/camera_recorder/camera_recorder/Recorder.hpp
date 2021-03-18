//
// Recorder.hpp
//
// Created on: Mar 18, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_CAMERARECORDER_HPP
#define CAMERA_RECORDER_CAMERA_RECORDER_CAMERARECORDER_HPP

extern "C" {
#include "avilib.h"
}

#include <condition_variable>
#include <array>
#include <etl/circular_buffer.h>
#include "utility/Subscription.hpp"

namespace CameraRecorder {

class Recorder {
public:
	void operator()();
	void newFrame(const std::shared_ptr<Ov2640::Image> &);
	void recordStart(const std::string &filename);
	void recordStop();
	Recorder();
private:
	static constexpr std::size_t kRingBufferCapacity = 2;
	static constexpr std::size_t kFrameCountRequired = 100;

	bool fileOpen();
	void fileWriteFrame();
	void fileWriteFps();
	void fileClose();

	struct {
		etl::circular_buffer<std::shared_ptr<Ov2640::Image>, kRingBufferCapacity> buffer;
		std::mutex mutex;
		struct {
			std::chrono::microseconds started;
			size_t count;
			std::chrono::microseconds stopped;
		} count;
	} frame;

	struct {
		bool running;
		struct {
			avi_t *descriptor;
			std::string name;
		} file;
		struct {
			std::condition_variable conditionVariable;
			std::mutex mutex;
		} sync;
	} record;

	struct {
		Utility::Subscription::Key::NewFrame    newFrame;
		Utility::Subscription::Key::RecordStart recordStart;
		Utility::Subscription::Key::RecordStop  recordStop;
	} key;
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_CAMERARECORDER_HPP
