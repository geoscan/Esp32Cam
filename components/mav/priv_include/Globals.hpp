//
// Globals.hpp
//
// Created on: Dec 23, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_GLOBALS_HPP
#define MAV_PRIV_INCLUDE_GLOBALS_HPP

#include "Mavlink.hpp"
#include <cstdint>

namespace Mav {

///
/// \brief Globally defined mavlink artifacts such as configuration parameters,
/// identifiers, etc.
///
struct Globals {
private:
	static constexpr auto kMaxPayloadLength = 255;
	static constexpr auto kMaxMavlinkMessageLength = 280;

public:
	static constexpr unsigned char getSysId()
	{
		return 1;
	}

	static constexpr unsigned char getCompId()
	{
		return MAV_COMP_ID_UDP_BRIDGE;
	}

	template <class TmavlinkMessage>
	static constexpr std::size_t getMaxMessageLength()
	{
		return kMaxMavlinkMessageLength - kMaxPayloadLength + sizeof(TmavlinkMessage);
	}
};

}  // namespace Mav

#endif  // MAV_PRIV_INCLUDE_GLOBALS_HPP
