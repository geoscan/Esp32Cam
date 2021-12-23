//
// GsNetwork.hpp
//
// Created on: Dec 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP
#define MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP

#include "Microservice.hpp"

struct __mavlink_mav_gs_network_t;
using mavlink_mav_gs_network_t = __mavlink_mav_gs_network_t;

namespace Utility {
namespace Subscription {

struct IpEndpointCommand;

}  // namespace Subscription
}  // namespace Utility

namespace Mav {
namespace Mic {

class GsNetwork final : public Microservice {
public:
	Ret process(mavlink_message_t &aMavlinkMessage) override;

private:
	Ret processConnectDisconnect(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	Ret processSend(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	Ret processReceived(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
};

}  // namespace Mic
}  // namespace Mav

#endif  // MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP
