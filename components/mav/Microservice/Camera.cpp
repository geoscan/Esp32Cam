//
// Camera.cpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "Microservice/Camera.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "sub/Rout.hpp"
#include "mav/mav.hpp"
#include "Globals.hpp"
#include "sub/Sys.hpp"

namespace Mav {
namespace Mic {

void Camera::onHrTimer()
{
	mavlink_heartbeat_t mavlinkHeartbeat;
	mavlinkHeartbeat.type = MAV_TYPE_CAMERA;
	mavlinkHeartbeat.autopilot = MAV_AUTOPILOT_INVALID;
	mavlinkHeartbeat.base_mode = 0;
	mavlinkHeartbeat.custom_mode = 0;
	mavlinkHeartbeat.system_status = MAV_STATE_STANDBY;

	mavlink_message_t mavlinkMessage;
	mavlink_msg_heartbeat_encode(Globals::getSysId(), Globals::getCompIdCamera(), &mavlinkMessage, &mavlinkHeartbeat);
	notify(mavlinkMessage);
}

Camera::Camera() : HrTimer{ESP_TIMER_TASK, "MavHbeat"}
{
	using namespace Sub::Sys;

	bool fCameraInitialized = false;

	for (auto &cb : Fld::ModuleGetField::getIterators()) {
		auto resp = cb(Fld::Req{Module::Camera, Fld::Field::Initialized});

		if (resp.tryGet<Module::Camera, Fld::Field::Initialized>(fCameraInitialized)) {
			ESP_LOGI(Mav::kDebugTag, "Microservice::Camera: Heartbeat emit, found camera module");
		}
	}

	if (fCameraInitialized) {
		startPeriodic(std::chrono::seconds(1));   // 1 Hz, https://mavlink.io/en/services/camera.html#camera-connection
	} else {
		ESP_LOGW(Mav::kDebugTag, "Microservice::Camera: Heartbeat emit, camera module was not initialized");
	}
}

Microservice::Ret Camera::process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse)
{
	Microservice::Ret ret = Microservice::Ret::Ignored;

	switch (aMessage.msgid) {
		case MAVLINK_MSG_ID_COMMAND_LONG:  // Falls through
		case MAVLINK_MSG_ID_COMMAND_INT: {

			mavlink_command_long_t commandLong = Hlpr::MavlinkCommandLong::makeFrom(aMessage);

			switch (commandLong.command) {

				case MAV_CMD_REQUEST_MESSAGE:
					switch (static_cast<int>(commandLong.param1)) {
						case MAVLINK_MSG_ID_CAMERA_INFORMATION:
							ret = processRequestMessageCameraInformation(commandLong, aMessage, aOnResponse);

							break;

						default:
							break;
					}

				break;
			}
		}

		default:

			break;
	}

	return ret;
}

Microservice::Ret Camera::processRequestMessageCameraInformation(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMavlinkMessage, Microservice::OnResponseSignature aOnResponse)
{
	assert(static_cast<int>(aMavlinkCommandLong.param1) == MAVLINK_MSG_ID_CAMERA_INFORMATION);
	using namespace Sub::Sys;
	ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation");

	struct {
		int sysid;
		int compid;
	} sender {aMavlinkMessage.sysid, aMavlinkMessage.compid};

	typename Fld::GetType<Fld::Field::Initialized, Module::Camera>::Type initialized = false;
	typename Fld::GetType<Fld::Field::FrameSize, Module::Camera>::Type frameSize{};

	for (auto &cb : Fld::ModuleGetField::getIterators()) {
		if (cb({Module::Camera, Fld::Field::Initialized}).tryGet<Module::Camera, Fld::Field::Initialized>(initialized)) {
			cb({Module::Camera, Fld::Field::FrameSize}).tryGet<Module::Camera, Fld::Field::FrameSize>(frameSize);
		}
	}

	{
		mavlink_command_ack_t mavlinkCommandAck {};
		mavlinkCommandAck.target_component = sender.compid;
		mavlinkCommandAck.target_system = sender.sysid;
		mavlinkCommandAck.result = initialized ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED;
		mavlinkCommandAck.command = aMavlinkCommandLong.command;

		mavlink_msg_command_ack_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage, &mavlinkCommandAck);
		ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation - packing `COMMAND_ACK`");
		aOnResponse(aMavlinkMessage);
	}

	if (initialized) {
		mavlink_camera_information_t mavlinkCameraInformation {};
		mavlinkCameraInformation.resolution_h = frameSize.first;
		mavlinkCameraInformation.resolution_v = frameSize.second;

		mavlink_msg_camera_information_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage,
			&mavlinkCameraInformation);
		ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation - packing `CAMERA_INFORMATION`");
		aOnResponse(aMavlinkMessage);
	}

	return Microservice::Ret::Response;
}

}  // namespace Mic
}  // namespace Mav
