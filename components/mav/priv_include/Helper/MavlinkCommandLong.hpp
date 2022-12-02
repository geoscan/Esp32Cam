//
// MavlinkCommandLong.hpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_HELPER_MAVLINKMESSAGECOMMAND_HPP_
#define MAV_PRIV_INCLUDE_HELPER_MAVLINKMESSAGECOMMAND_HPP_

#include "Mavlink.hpp"
#include "Common.hpp"

namespace Mav {
namespace Hlpr {

struct MavlinkCommandLong : mavlink_command_long_t, Cmn::Impl::Pack<mavlink_command_long_t> {
	static MavlinkCommandLong makeFrom(const mavlink_command_int_t &);
	static MavlinkCommandLong makeFrom(const mavlink_message_t &);
	MavlinkCommandLong &initTargetFromRequest(const mavlink_message_t &aRequest);
	MavlinkCommandLong &initRequestMessage(unsigned aMsgid, float aParam2 = 0.0f, float aParam3 = 0.0f,
		float aParam4 = 0.0f, float aParam5 = 0.0f);
};

}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_MAVLINKMESSAGECOMMAND_HPP_
