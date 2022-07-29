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
#include "wifi_uart_bridge/Receiver.hpp"

struct __mavlink_mav_gs_network_t;
using mavlink_mav_gs_network_t = __mavlink_mav_gs_network_t;

namespace Utility {
namespace Subscription {

struct IpEndpointCommand;

}  // namespace Subscription
}  // namespace Utility

namespace Mav {
namespace Mic {

class GsNetwork final : public Microservice, public Bdg::Receiver {
public:
	GsNetwork();
	Ret process(mavlink_message_t &aMavlinkMessage, OnResponseSignature) override;

private:
	template <class Tproto, class Tbuf>
	std::size_t initMavlinkMavGsNetwork(mavlink_mav_gs_network_t &mavGsNetwork,
		const asio::ip::basic_endpoint<Tproto> &aRemoteEndpoint, std::uint16_t aLocalPort, Tbuf &&aBuffer);

	template <class Tproto>
	typename Sub::Rout::MavlinkPackForward<Tproto>::Ret packForward(
		typename Sub::Rout::MavlinkPackForward<Tproto>::template Arg<0>);

	typename Sub::Rout::MavlinkPackTcpEvent::Ret packTcpEvent(typename Sub::Rout::MavlinkPackTcpEvent::Arg<0>);
	Sub::Rout::Response respPackLock(const mavlink_mav_gs_network_t &);
	static std::size_t mavGsNetworkGetMaxMessageLength(std::size_t aHintPayloadLength);
	asio::ip::address getAddress(mavlink_mav_gs_network_t &);
	asio::const_buffer getBuffer(mavlink_mav_gs_network_t &);
	void processConnect(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processDisconnect(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processOpen(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processClose(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processSend(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);
	void processReceived(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork);

private:
	void onReceive(Bdg::OnReceiveCtx aCtx) override;

private:
	struct Key {
		typename Sub::Rout::MavlinkPackForward<asio::ip::tcp> packForwardTcp;
		typename Sub::Rout::MavlinkPackForward<asio::ip::udp> packForwardUdp;
		Sub::Rout::MavlinkPackTcpEvent packTcpEvent;
	} key;

	struct {
		std::uint8_t buffer[sizeof(mavlink_message_t)];  // Response buffer for TCP forward
		std::mutex mutex;
	} resp;
};

}  // namespace Mic
}  // namespace Mav

#include "GsNetwork.impl"

#endif  // MAV_PRIV_INCLUDE_MICROSERVICE_GSNETWORK_HPP
