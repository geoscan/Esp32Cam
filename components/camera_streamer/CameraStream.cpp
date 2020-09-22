#include <freertos/FreeRTOS.h>
//
// CameraStream.cpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_timer.h>

#include <memory>

#include "CameraStream.hpp"
#include "Ov2640.hpp"

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
	static const auto kWaitMs = (fps > 0) ? 1000 / fps : 0;

	auto img = Ov2640::instance().jpeg(); // Trigger HW-initialization

	while(true) {
		lock();
		if (sinks.empty()) {
			unlock();
		} else {
			auto lastSend = currentTimeMs();
			for (auto &sink : sinks) {
				socket.send_to(img->data(), sink);
			}
			unlock();

			img = Ov2640::instance().jpeg();

			if (fps > 0) {
				auto timeDelta = currentTimeMs() - lastSend;
				// Timer counter overflow and high latency are taken into accout
				auto timeWait = (timeDelta > 0 && timeDelta < kWaitMs) ? kWaitMs - timeDelta : 0;
				vTaskDelay((timeWait) / portTICK_PERIOD_MS);
			}
		}
	}
}

void CameraStream::addSink(udp::endpoint ep)
{
	mutex.lock();
	sinks.insert(ep);
	mutex.unlock();
}

void CameraStream::removeSink(udp::endpoint ep)
{
	mutex.lock();
	sinks.erase(ep);
	mutex.unlock();
}

void CameraStream::removeSinks()
{
	mutex.lock();
	sinks.clear();
	mutex.unlock();
}

void CameraStream::lock()
{
	mutex.lock();
}

void CameraStream::unlock()
{
	mutex.unlock();
}