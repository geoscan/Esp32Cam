//
// MavlinkCommandAck.hpp
//
// Created on: Apr 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_HELPER_MAVLINKCOMMANDACK_HPP_
#define MAV_PRIV_INCLUDE_HELPER_MAVLINKCOMMANDACK_HPP_

#include "Mavlink.hpp"
#include "Common.hpp"

namespace Mav {
namespace Hlpr {

struct MavlinkCommandAck : Cmn::Impl::Pack<mavlink_command_ack_t> {
	static MavlinkCommandAck makeFrom(const mavlink_message_t &aMessage,
		decltype(mavlink_command_ack_t::command) aCommand,
		decltype(mavlink_command_ack_t::result) aResult = MAV_RESULT_ACCEPTED);
	void packInto(mavlink_message_t &aInto);
};

}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_MAVLINKCOMMANDACK_HPP_
