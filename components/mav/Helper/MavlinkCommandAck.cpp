//
// MavlinkCommandAck.cpp
//
// Created on: Apr 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Helper/MavlinkCommandAck.hpp"
#include "Globals.hpp"

namespace Mav {
namespace Hlpr {

MavlinkCommandAck MavlinkCommandAck::makeFrom(const mavlink_message_t &aMessage,
	decltype(mavlink_command_ack_t::command) aCommand,
	decltype(mavlink_command_ack_t::result) aResult)
{
	MavlinkCommandAck ret {};
	ret.target_system = aMessage.sysid;
	ret.target_component = aMessage.compid;
	ret.command = aCommand;
	ret.result = aResult;

	return ret;
}

void MavlinkCommandAck::packInto(mavlink_message_t &aInto)
{
	mavlink_msg_command_ack_encode(Globals::getSysId(), Globals::getCompId(), &aInto, this);
}

}  // namespace Hlpr
}  // namespace Mav
