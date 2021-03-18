//
// CameraRecorder.cpp
//
// Created on: Mar 18, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/Recorder.hpp"
#include "utility/time.hpp"

using namespace CameraRecorder;

Recorder::Recorder() :
	key {{&Recorder::newFrame, this},
		{&Recorder::recordStart, this},
		{&Recorder::recordStop, this}}
{
}

void Recorder::operator ()()
{
	while (true) {
		std::unique_lock<std::mutex> lock(record.sync.mutex);
		lock.lock();
		if (!record.running) {
			lock.unlock();
			record.sync.conditionVariable.wait(lock, [&](){return record.running;});  // Efficient wait for condition
		}

		lock.lock();
		if (record.running && fileOpen()) {
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
			fileWriteFps();  // Calculate FPS and write results to file
			fileClose();
		}
	}
}

void Recorder::newFrame(const std::shared_ptr<Ov2640::Image> &image)
{
	std::lock_guard<std::mutex> lock(frame.mutex);
	frame.buffer.push(image);
}

void Recorder::recordStart(const std::string &filename)
{
	{
		std::lock_guard<std::mutex> lock(record.sync.mutex);
		record.running = true;
		record.file.name = std::string(CONFIG_SD_FAT_MOUNT_POINT"/") + std::string("filename");
	}
	record.sync.conditionVariable.notify_all();
}

void Recorder::recordStop()
{
	{
		std::lock_guard<std::mutex> lock(record.sync.mutex);
		record.running = false;
	}
}

bool Recorder::fileOpen()
{
	record.file.descriptor = AVI_open_output_file(const_cast<char *>(record.file.name.c_str()));
	return record.file.descriptor != nullptr;
}

void Recorder::fileWriteFrame()
{
	frame.mutex.lock();
	if (!frame.buffer.empty()) {
		std::shared_ptr<Ov2640::Image> f = frame.buffer.back();  // TODO: back?
		frame.buffer.pop();
		frame.mutex.unlock();

		AVI_write_frame(record.file.descriptor, reinterpret_cast<char *>(const_cast<void *>(f->data().data())),
			f->data().size());
	}
}

void Recorder::fileWriteFps()
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

void Recorder::fileClose()
{
	if (record.file.descriptor) {
		AVI_close(record.file.descriptor);
	}
}