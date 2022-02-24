//
// GsNetwork.hpp
//
// Created on: Dec 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP
#define MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP

#include "Microservice.hpp"
#include "sub/Rout.hpp"

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
	GsNetwork();
	Ret process(mavlink_message_t &aMavlinkMessage) override;

private:
	asio::ip::address getAddress(mavlink_mav_gs_network_t &);
	asio::const_buffer getBuffer(mavlink_mav_gs_network_t &);
	Sub::Rout::MavlinkPackForward::Ret packForward(typename Sub::Rout::MavlinkPackForward::Arg<0>);
	void processConnect(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processDisconnect(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processOpen(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processClose(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processSend(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processReceived(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);

private:
	struct Key {
		Sub::Rout::MavlinkPackForward packForward;
	} key;
};

}  // namespace Mic
}  // namespace Mav


#endif  // MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP
