//
// MavlinkCommandLong.hpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_HELPER_MAVLINKMESSAGECOMMAND_HPP_
#define MAV_PRIV_INCLUDE_HELPER_MAVLINKMESSAGECOMMAND_HPP_

#include "Mavlink.hpp"

namespace Mav {
namespace Hlpr {

struct MavlinkCommandLong : mavlink_command_long_t {
	static mavlink_command_long_t makeFrom(const mavlink_command_int_t &);
};

}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_MAVLINKMESSAGECOMMAND_HPP_
