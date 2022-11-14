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
#include "Ov2640.hpp"
#include <esp_log.h>

using asio::ip::udp;
using namespace std;
using namespace CameraThread;

CameraStream::CameraStream(Fps f) :	fps(f)
{
}

void CameraStream::operator()()
{
	using Time = decltype(Ut::bootTimeUs());
	static const auto kWaitMs = (fps > 0) ? 1000 / fps : 0;

	auto img = Cam::Camera::getInstance().getFrame();

	Time lastSend = 0;

	while(true) {
		if (!img.get()) {
			ESP_LOGW("[camera_thread]", "skipping nullptr frame");
			img.reset();
			img = Cam::Camera::getInstance().getFrame();
		} else if (!img.get()->valid()) {
			ESP_LOGW("[camera_thread]", "skipping invalid frame");
			img.reset();
			img = Cam::Camera::getInstance().getFrame();
		} else {
			if (fps > 0) {
				lastSend = Ut::bootTimeUs() / 1000;
			}

			key.notify(img);
			img.reset();
			img = Cam::Camera::getInstance().getFrame();

			if (fps > 0) {
				// Timer counter overflow and high latency are taken into account
				auto timeDelta = Ut::bootTimeUs() / 1000 - lastSend;
				auto timeWait = (timeDelta > 0 && timeDelta < kWaitMs) ? kWaitMs - timeDelta : 0;
				Ut::waitMs(timeWait);
			}

		}
	}
}
