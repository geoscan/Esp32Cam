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
	res.param5 = static_cast<decltype(float)>(aIn.x);
	res.param6 = static_cast<decltype(float)>(aIn.y);
	res.param7 = aIn.z;
	res.command = aIn.command;

	return res;
}

}  // namespace Hlpr
}  // namespace Mav
