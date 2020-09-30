//
// CameraStream.cpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

#include <memory>

#include "CameraStream.hpp"
#include "Ov2640.hpp"
#include "utility/time.hpp"
#include "utility/LockGuard.hpp"

using asio::ip::udp;
using namespace std;

int64_t CameraStream::currentTimeMs()
{
	return esp_timer_get_time() / 1000;
}

CameraStream::CameraStream(asio::io_context &context, uint16_t sourcePort, Fps f) :
	socket(context, udp::endpoint(udp::v4(), sourcePort)),
	fps(f)
{
}

void CameraStream::run()
{
	using Time = decltype(currentTimeMs());
	static const unsigned kWaitForConnectionMs = 500;
	static const auto kWaitMs = (fps > 0) ? 1000 / fps : 0;

	auto img = Ov2640::instance().jpeg(); // Trigger HW-initialization

	Time lastSend = 0;

	while(true) {
		lock();
		if (sinks.empty()) {
			unlock();
			Utility::waitMs(kWaitForConnectionMs);  // To prevent resource starvation
		} else {
			if (fps > 0) {
				lastSend = currentTimeMs();
			}
			for (auto &sink : sinks) {
				asio::error_code err;
				socket.send_to(img->data(), sink, 0, err);
			}
			unlock();

			img = Ov2640::instance().jpeg();

			if (fps > 0) {
				// Timer counter overflow and high latency are taken into account
				auto timeDelta = currentTimeMs() - lastSend;
				auto timeWait = (timeDelta > 0 && timeDelta < kWaitMs) ? kWaitMs - timeDelta : 0;
				vTaskDelay((timeWait) / portTICK_PERIOD_MS);
			}
		}
	}
}

void CameraStream::addSink(udp::endpoint ep)
{
	auto lock = Utility::makeLockGuard(mutex);
	sinks.insert(ep);
}

void CameraStream::removeSink(udp::endpoint ep)
{
	auto lock = Utility::makeLockGuard(mutex);
	sinks.erase(ep);
}

void CameraStream::removeSinks()
{
	auto lock = Utility::makeLockGuard(mutex);
	sinks.clear();
}

void CameraStream::lock()
{
	mutex.lock();
}

void CameraStream::unlock()
{
	mutex.unlock();
}