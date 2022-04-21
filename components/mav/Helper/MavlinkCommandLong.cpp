//
// MavlinkCommandLong.cpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Helper/MavlinkCommandLong.hpp"

namespace Mav {
namespace Hlpr {

mavlink_command_long_t MavlinkCommandLong::makeFrom(const mavlink_command_int_t &aIn)
{
	mavlink_command_long_t res;

	res.target_component = aIn.target_component;
	res.target_system = aIn.target_system;
	res.param1 = aIn.param1;
	res.param2 = aIn.param2;
	res.param3 = aIn.param3;
	res.param4 = aIn.param4;
	res.param5 = static_cast<float>(aIn.x);
	res.param6 = static_cast<float>(aIn.y);
	res.param7 = aIn.z;
	res.command = aIn.command;

	return res;
}

mavlink_command_long_t MavlinkCommandLong::makeFrom(const mavlink_message_t &aMessage)
{
	mavlink_command_long_t ret;

	if (MAVLINK_MSG_ID_COMMAND_LONG == aMessage.msgid) {
		mavlink_msg_command_long_decode(&aMessage, &ret);
	} else if (MAVLINK_MSG_ID_COMMAND_INT == aMessage.msgid) {
		mavlink_command_int_t mavlinkCommandInt;
		mavlink_msg_command_int_decode(&aMessage, mavlinkCommandInt);
		ret = makeFrom(mavlinkCommandInt);
	}

	return ret;
}

}  // namespace Hlpr
}  // namespace Mav
