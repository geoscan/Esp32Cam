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

CameraStream::CameraStream(asio::io_context &context, uint16_t sourcePort, Fps f) :
	socket(context, udp::endpoint(udp::v4(), sourcePort)),
	fps(f)
{
}

void CameraStream::run()
{
	using Time = decltype(Utility::bootTimeUs());
	static const unsigned kWaitForConnectionMs = 500;
	static const auto kWaitMs = (fps > 0) ? 1000 / fps : 0;

	auto img = Ov2640::instance().jpeg(); // Trigger HW-initialization

	Time lastSend = 0;

	while(true) {
		mutex.lock();
		if (sinks.empty()) {
			mutex.unlock();
			Utility::waitMs(kWaitForConnectionMs);  // To prevent resource starvation
		} else {
			if (fps > 0) {
				lastSend = Utility::bootTimeUs() / 1000;
			}
			for (auto &sink : sinks) {
				asio::error_code err;
				socket.send_to(img->data(), udp::endpoint{sink.first, sink.second}, 0, err);
			}
			mutex.unlock();

			img = Ov2640::instance().jpeg();

			if (fps > 0) {
				// Timer counter overflow and high latency are taken into account
				auto timeDelta = Utility::bootTimeUs() / 1000 - lastSend;
				auto timeWait = (timeDelta > 0 && timeDelta < kWaitMs) ? kWaitMs - timeDelta : 0;
				Utility::waitMs(timeWait);
			}
		}
	}
}

void CameraStream::addSink(const asio::ip::address &addr, short unsigned port)
{
	auto lock = Utility::makeLockGuard(mutex);
	auto it = sinks.find(addr);
	if (it != sinks.end()) {
		sinks.erase(it);
	}
	sinks.insert({addr, port});
}

void CameraStream::removeSink(const asio::ip::address &addr)
{
	auto lock = Utility::makeLockGuard(mutex);
	auto it = sinks.find(addr);
	if (it != sinks.end()) {
		sinks.erase(it);
	}
}

void CameraStream::removeSinks()
{
	auto lock = Utility::makeLockGuard(mutex);
	sinks.clear();
}