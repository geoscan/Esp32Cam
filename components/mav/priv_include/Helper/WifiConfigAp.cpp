//
// WifiConfigAp.cpp
//
// Created on: Dec 06, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "WifiConfigAp.hpp"
#include <algorithm>

namespace Mav {
namespace Hlpr {

void WifiConfigAp::ssidFillZero()
{
	std::fill_n(ssid, sizeof(ssid), 0);
}

}  // namespace Hlpr
}  // namespace Mav
