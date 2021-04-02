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
#include "utility/time.hpp"
#include "utility/LockGuard.hpp"
#include "Ov2640.hpp"

using asio::ip::udp;
using namespace std;
using namespace CameraStreamer;

CameraStream::CameraStream(Fps f) :	fps(f)
{
}

void CameraStream::operator()()
{
	using Time = decltype(Utility::bootTimeUs());
	static const auto kWaitMs = (fps > 0) ? 1000 / fps : 0;

	auto img = Cam::Camera::getInstance().getFrame();

	Time lastSend = 0;

	while(true) {
		if (fps > 0) {
			lastSend = Utility::bootTimeUs() / 1000;
		}

		key.notify(img);
		img.reset();
		img = Cam::Camera::getInstance().getFrame();

		if (fps > 0) {
			// Timer counter overflow and high latency are taken into account
			auto timeDelta = Utility::bootTimeUs() / 1000 - lastSend;
			auto timeWait = (timeDelta > 0 && timeDelta < kWaitMs) ? kWaitMs - timeDelta : 0;
			Utility::waitMs(timeWait);
		}
	}
}