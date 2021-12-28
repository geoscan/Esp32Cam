//
// Globals.hpp
//
// Created on: Dec 23, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_GLOBALS_HPP
#define MAV_PRIV_INCLUDE_GLOBALS_HPP

#include "Mavlink.hpp"

namespace Mav {

///
/// \brief Globally defined mavlink artifacts such as configuration parameters,
/// identifiers, etc.
///
struct Globals {
	static constexpr unsigned char getSysId()
	{
		return 1;
	}

	static constexpr unsigned char getCompId()
	{
		return MAV_COMP_ID_UDP_BRIDGE;
	}
};

}  // namespace Mav

#endif  // MAV_PRIV_INCLUDE_GLOBALS_HPP
