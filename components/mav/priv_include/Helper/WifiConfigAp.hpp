//
// WifiConfigAp.hpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MAV_PRIV_INCLUDE_HELPER_MAVLINKCONFIGAP_HPP_)
#define MAV_PRIV_INCLUDE_HELPER_MAVLINKCONFIGAP_HPP_

#include "Mavlink.hpp"
#include "Common.hpp"

namespace Mav {
namespace Hlpr {

struct WifiConfigAp : mavlink_wifi_config_ap_t, Cmn::Impl::Pack<mavlink_command_long_t> {
};

}  // namespace Hlpr
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_HELPER_MAVLINKCONFIGAP_HPP_
