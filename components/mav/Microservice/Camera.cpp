//
// Camera.cpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Microservice/Camera.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "sub/Rout.hpp"
#include "Globals.hpp"

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
	mavlink_msg_heartbeat_encode(Globals::getSysId(), Globals::getCompId(), &mavlinkMessage, &mavlinkHeartbeat);
	notify(mavlinkMessage);
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

	struct {
		int sysid;
		int compid;
	} sender {aMavlinkMessage.sysid, aMavlinkMessage.compid};

	{
		mavlink_command_ack_t mavlinkCommandAck;
		mavlinkCommandAck.target_component = sender.compid;
		mavlinkCommandAck.target_system = sender.sysid;
		mavlinkCommandAck.result = MAV_CMD_ACK_OK;
		mavlinkCommandAck.result_param2 = 0;
		mavlinkCommandAck.progress = 0;
		mavlinkCommandAck.command = aMavlinkCommandLong.command;

		mavlink_msg_command_ack_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage, &mavlinkCommandAck);
		aOnResponse(aMavlinkMessage);
	}

	{
		mavlink_camera_information_t mavlinkCameraInformation {};
		mavlink_msg_camera_information_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage,
			&mavlinkCameraInformation);
		aOnResponse(aMavlinkMessage);
	}

	return Microservice::Ret::Response;
}

}  // namespace Mic
}  // namespace Mav
