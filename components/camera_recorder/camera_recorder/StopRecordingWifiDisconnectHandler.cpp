//
// StopRecordingWifiDisconnectHandler.cpp
//
// Created on: Aug 08, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "camera_recorder.hpp"
#include "wifi.h"

#include "StopRecordingWifiDisconnectHandler.hpp"

namespace CameraRecorder {

static constexpr const char *kLogPreamble = "CameraRecorder::StopRecordingWifiDisconnectHandler";

StopRecordingWifiDisconnectHandler::StopRecordingWifiDisconnectHandler() :
	subscription{
		{&StopRecordingWifiDisconnectHandler::onWifiDisconnected, this}
	}
{
}

void StopRecordingWifiDisconnectHandler::onHrTimer()
{
	unsigned long nConnectedStations = 0;
	const auto espError = wifiApCountConnectedStations(&nConnectedStations);

	// The client has failed to reestablish connection
	if (espError == ESP_OK && nConnectedStations == 0) {
		if (record != nullptr) {
			ESP_LOGI(CameraRecorder::kDebugTag, "%s::%s: issuing record stop", kLogPreamble, __func__);
			record->stop();
		} else {
			ESP_LOGE(CameraRecorder::kDebugTag, "%s::%s: no `Record` instance has been registered (nullptr)",
				kLogPreamble, __func__);
		}
	}
}

void StopRecordingWifiDisconnectHandler::onWifiDisconnected(asio::ip::address)
{
	stopTimer();
	startOnce(kReestablishWifiConnectionTimeout);
}

}  // CameraRecorder
