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

uint32_t CameraStream::currentTimeMs()
{
	return static_cast<uint32_t>(esp_timer_get_time() * 1000);
}

CameraStream::CameraStream(asio::io_context &context, uint16_t sourcePort, Fps f) :
	socket(context, udp::endpoint(udp::v4(), sourcePort)),
	fps(f)
{
}

void CameraStream::run()
{
	using Time = decltype(currentTimeMs());
	static const auto kWaitMs = 1000 / fps;

	auto img = Ov2640::instance().jpeg();
	Time timeLast;

	while(true) {
		mutex.lock();
		if (!sinks.empty()) {
			mutex.unlock();

			// Send JPEG
			if (fps) {
				timeLast = currentTimeMs();
			}

			mutex.lock();
			for (auto &sink : sinks) {
				socket.send_to(img->data(), sink);
			}
			mutex.unlock();

			img = Ov2640::instance().jpeg();

			if (fps) {
				// Wait until we are eligible for sending the next frame
				auto timeNow = currentTimeMs();
				auto timedelta = (timeNow < timeLast /*overflow*/) ? kWaitMs / 2 : timeNow - timeLast;
				vTaskDelay((kWaitMs - timedelta) / portTICK_PERIOD_MS);
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