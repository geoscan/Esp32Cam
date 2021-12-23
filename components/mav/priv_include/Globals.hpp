//
// Globals.hpp
//
// Created on: Dec 23, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_GLOBALS_HPP
#define MAV_PRIV_INCLUDE_GLOBALS_HPP

namespace Mav {

///
/// \brief Globally defined mavlink artifacts such as configuration parameters,
/// identifiers, etc.
///
struct Globals {
	constexpr unsigned char getSysId();
	constexpr unsigned char getCompId();
};

}  // namespace Mav

#endif  // MAV_PRIV_INCLUDE_GLOBALS_HPP
