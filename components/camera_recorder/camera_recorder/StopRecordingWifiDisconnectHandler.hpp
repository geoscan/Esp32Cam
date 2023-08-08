//
// StopRecordingWifiDisconnectHandler.hpp
//
// Created on: Aug 08, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_STOPRECORDINGWIFIDISCONNECTHANDLER_HPP
#define CAMERA_RECORDER_CAMERA_RECORDER_STOPRECORDINGWIFIDISCONNECTHANDLER_HPP

#include "camera_recorder/RecFrame.hpp"
#include "utility/system/HrTimer.hpp"
#include "wifi/Sub.hpp"

namespace CameraRecorder {

/// Stops recording after a certain timeout, if the last Wi-Fi has
/// disconnected, and no one has established a connection by the moment the
/// timeout is triggered.
class StopRecordingWifiDisconnectHandler : public Ut::Sys::HrTimer {
private:
	static constexpr std::chrono::seconds kReestablishWifiConnectionTimeout{45};

	/// Subscription keys package
	struct Subscription {
		Wifi::Sub::Disconnected onWifiDisconnected;

		Subscription(StopRecordingWifiDisconnectHandler &aOwner);
		~Subscription();
	};

public:
	StopRecordingWifiDisconnectHandler(Record *aRecord);
	~StopRecordingWifiDisconnectHandler() = default;

	/// Handle timeout: stop the recording
	void onHrTimer() override;

	/// Handle Wi-Fi disconnected event
	void onWifiDisconnected(asio::ip::address);

private:
	Subscription subscription;
	Record *record;
};

}  // CameraRecorder

#endif // CAMERA_RECORDER_CAMERA_RECORDER_STOPRECORDINGWIFIDISCONNECTHANDLER_HPP
