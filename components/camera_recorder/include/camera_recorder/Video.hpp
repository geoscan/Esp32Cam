//
// Video.hpp
//
// Created on: Mar 18, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// Runs continuously, receives frames, writes them to a file. Works as a sink,
// meaning that there must be a stream of frames. As for Mar 24, 2021,
// CameraStreamer::CameraStream is used as the stream source.
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
#include "camera_recorder/Recorder.hpp"

namespace CameraRecorder {

class Video : public Recorder {
public:
	void operator()();
	void newFrame(const std::shared_ptr<Ov2640::Image> &) override;
	void recordStart(const std::string &filename) override;
	void recordStop() override;

	using Recorder::Recorder;
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
};

}  // namespace CameraRecorder

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_CAMERARECORDER_HPP
