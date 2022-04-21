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
		case MAVLINK_MSG_ID_COMMAND_LONG: {
			mavlink_command_long_t mavlinkCommandLong;
			(void)mavlinkCommandLong;
			mavlink_msg_command_long_decode(&aMessage, &mavlinkCommandLong);
			ret = processCommand(mavlinkCommandLong, aMessage, aOnResponse);

			break;
		}

		case MAVLINK_MSG_ID_COMMAND_INT: {
			mavlink_command_int_t mavlinkCommandInt;
			(void)mavlinkCommandInt;
			mavlink_msg_command_int_decode(&aMessage, &mavlinkCommandInt);
			mavlink_command_long_t mavlinkCommandLong = Hlpr::MavlinkCommandLong::makeFrom(mavlinkCommandInt);
			ret = processCommand(mavlinkCommandLong, aMessage, aOnResponse);

			break;
		}

		default:

			break;
	}

	return ret;
}

Microservice::Ret Camera::processCommand(mavlink_command_long_t &aMavlinkCommandLong, mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	return {};
}

}  // namespace Mic
}  // namespace Mav
