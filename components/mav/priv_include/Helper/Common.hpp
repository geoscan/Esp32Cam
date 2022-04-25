//
// Common.hpp
//
// Created on: Apr 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_HELPER_COMMON_HPP_
#define MAV_PRIV_INCLUDE_HELPER_COMMON_HPP_

#include "utility/time.hpp"
#include <chrono>

namespace Mav {
namespace Hlpr {
namespace Cmn {

template <class T>
void fieldTimeBootMsInit(T &aObj)
{
	aObj.time_boot_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::microseconds{Utility::bootTimeUs()}).count();
}

}  // namespace Cmn
}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_COMMON_HPP_
