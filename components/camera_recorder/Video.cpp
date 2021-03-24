//
// Video.cpp
//
// Created on: Mar 18, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_log.h>
#include "camera_recorder/Video.hpp"
#include "utility/time.hpp"

static constexpr const char *kTag = "[camera_recorder, Video]";

using namespace CameraRecorder;

void Video::operator ()()
{
	std::unique_lock<std::mutex> lock(record.sync.mutex);
	if (!record.running) {
		record.sync.conditionVariable.wait(lock, [&](){return record.running;});  // Efficient wait for condition
	} else {
		return;
	}

	lock.lock();
	if (record.running && fileOpen()) {

		ESP_LOGI(kTag, "Rec. started, file: %s", record.file.name.c_str());

		frame.count.started = std::chrono::microseconds(Utility::bootTimeUs());

		for (; record.running; lock.lock()) {
			lock.unlock();

			// Write frame from ring buffer
			fileWriteFrame();

			// Update frame statistics, so it'll be possible to calculate FPS
			if (frame.count.count < kFrameCountRequired) {
				++frame.count.count;
			} else if (frame.count.count == kFrameCountRequired) {
				frame.count.stopped = std::chrono::microseconds(Utility::bootTimeUs());
			}
		}

		lock.lock();

		fileWriteFps();  // Calculate FPS and write results to the file
		fileClose();

		lock.unlock();
		record.sync.conditionVariable.notify_all();  // We're done with the file. Notify the thread that instantiated 'stop' request.
	}

	ESP_LOGI(kTag, "Rec stopped");
}

void Video::newFrame(const std::shared_ptr<Ov2640::Image> &image)
{
	// No mutexes require to access record.running. Frames get updated rather
	// fast. Therefore, there's no need to achieve complete synchonisation. It's
	// not a big deal, if we miss a frame or two, or neither is it if we write a
	// couple of redundant ones.

	if (!record.running) {
		return;
	}

	std::lock_guard<std::mutex> lock(frame.mutex);
	frame.buffer.push(image);
}

void Video::recordStart(const std::string &filename)
{
	{
		std::lock_guard<std::mutex> lock(record.sync.mutex);
		if (record.running) {
			return;
		}
		record.running = true;
		record.file.name = std::string(CONFIG_SD_FAT_MOUNT_POINT"/") + std::string("filename");
	}
	record.sync.conditionVariable.notify_all();
}

void Video::recordStop()
{
	std::unique_lock<std::mutex> lock(record.sync.mutex);
	record.running = false;
	if (record.file.descriptor != nullptr) {
		record.sync.conditionVariable.wait(lock, [&](){return record.file.descriptor == nullptr; });  // Wait until worker thread notifies us that it's done with the file
	}
}

bool Video::fileOpen()
{
	record.file.descriptor = AVI_open_output_file(const_cast<char *>(record.file.name.c_str()));
	return record.file.descriptor != nullptr;
}

void Video::fileWriteFrame()
{
	frame.mutex.lock();
	if (!frame.buffer.empty()) {
		std::shared_ptr<Ov2640::Image> f = frame.buffer.back();  // TODO: back?
		frame.buffer.pop();
		frame.mutex.unlock(); // Reference counter has been updated, no need keep the lock.

		AVI_write_frame(record.file.descriptor, reinterpret_cast<char *>(const_cast<void *>(f->data().data())),
			f->data().size());
	}
}

void Video::fileWriteFps()
{
	if (frame.count.count < kFrameCountRequired) {  // Means we haven't finished counting frames
		frame.count.stopped = std::chrono::microseconds(Utility::bootTimeUs());
	}

	float timeDiff = static_cast<float>((frame.count.stopped - frame.count.started).count());  // us
	timeDiff /= 1000000.0f;  // seconds

	constexpr const char *compressorType = "MJPG";
	// TODO: 640x480 - remove magic numbers
	AVI_set_video(record.file.descriptor, 640, 480, timeDiff, const_cast<char *>(compressorType));
}

void Video::fileClose()
{
	if (record.file.descriptor) {
		AVI_close(record.file.descriptor);
	}
}