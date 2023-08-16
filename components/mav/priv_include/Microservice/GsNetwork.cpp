//
// GsNetwork.cpp
//
// Created on: Dec 22, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "Mavlink.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Marshalling.hpp"
#include "Globals.hpp"
#include "sub/Subscription.hpp"
#include "socket/Api.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "utility/al/Algorithm.hpp"
#include "utility/LogSection.hpp"
#include <algorithm>
#include <memory>
#include "mav/mav.hpp"

GS_UTILITY_LOGD_METHOD_SET_ENABLED(Mav::Mic::GsNetwork, process, 1)

using namespace Mav;
using namespace Mav::Mic;
using namespace asio::ip;

GsNetwork::GsNetwork() :
	Bdg::Receiver{Bdg::NamedEndpoint::MavlinkIpPack, "MavlinkGsNetwork"},
	key{
		{&GsNetwork::onTcpEvent, this}
	}
{
}

Microservice::Ret GsNetwork::process(mavlink_message_t &aMavlinkMessage, OnResponseSignature aOnResponse)
{
	if (aMavlinkMessage.msgid != MAVLINK_MSG_ID_MAV_GS_NETWORK) {
		return Ret::Ignored;
	}

	mavlink_mav_gs_network_t mavlinkMavGsNetwork;
	Ret ret;
	mavlink_msg_mav_gs_network_decode(&aMavlinkMessage, &mavlinkMavGsNetwork);

	GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "mavlink_mav_gs_network_t: host_port %d remote_port: %d "
		"command %d ack %d transport %d payload_len %d, payload: %*.*s", mavlinkMavGsNetwork.host_port,
		mavlinkMavGsNetwork.remote_port, mavlinkMavGsNetwork.command, mavlinkMavGsNetwork.ack,
		mavlinkMavGsNetwork.transport, mavlinkMavGsNetwork.payload_len, mavlinkMavGsNetwork.payload_len,
		mavlinkMavGsNetwork.payload_len, mavlinkMavGsNetwork.payload);

	switch (mavlinkMavGsNetwork.ack) {
		case MAV_GS_NETWORK_ACK_NONE:
			ret = Ret::Response;
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "response required");

			break;

		case MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE:
			ret = Ret::NoResponse;
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "No response required");

			break;

		default:  // Non-request message
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "Non-request message");
			return Ret::NoResponse;
	}

	switch (mavlinkMavGsNetwork.command) {  // Command message
		case MAV_GS_NETWORK_COMMAND_CONNECT:
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "CONNECT");
			processConnect(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_DISCONNECT:
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "DISCONNECT");
			processDisconnect(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_SEND:
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "SEND");
			processSend(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_OPEN:
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "OPEN");
			processOpen(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_CLOSE:
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "CLOSE");
			processClose(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_PROCESS_RECEIVED:
			GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, GsNetwork, process, "COMMAND_PROCESS_RECEIVED");
			processReceived(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		default:
			return Ret::Ignored;
	}

	mavlink_msg_mav_gs_network_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage,
		&mavlinkMavGsNetwork);
	aOnResponse(aMavlinkMessage);

	return ret;
}

///
/// \brief GsNetwork::mavGsNetworkGetMaxMessageLength Calculates max. possible length of `mavlink_mav_gs_network_t`
/// after serialization.
///
/// \pre `::payload` field is zeroed
/// \pre In the original XML file defining the message, `::payload` field retains u8 type, and is defined the latest.
///
/// \param aHintPayloadLength Length of the message that is to be packed into `mavlink_mav_gs_network_t::payload`
///
std::size_t GsNetwork::mavGsNetworkGetMaxMessageLength(std::size_t aHintPayloadLength)
{
	return Globals::getMaxMessageLength<mavlink_mav_gs_network_t>() - sizeof(mavlink_mav_gs_network_t::payload)
		+ aHintPayloadLength;
}

asio::ip::address GsNetwork::getAddress(mavlink_mav_gs_network_t & aMavlinkMavGsNetwork)  ///< Extract address from payload field
{
	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_UDP4: {
			asio::ip::address_v4::bytes_type bytes = {{
				aMavlinkMavGsNetwork.payload[0],
				aMavlinkMavGsNetwork.payload[1],
				aMavlinkMavGsNetwork.payload[2],
				aMavlinkMavGsNetwork.payload[3]}};

			return asio::ip::make_address_v4(bytes);
		}

		case MAV_GS_NETWORK_TRANSPORT_TCP6:
		case MAV_GS_NETWORK_TRANSPORT_UDP6: {
			asio::ip::address_v6::bytes_type bytes;
			std::copy(aMavlinkMavGsNetwork.payload, aMavlinkMavGsNetwork.payload + 16, bytes.begin());

			return asio::ip::make_address_v6(bytes, 0);
		}

		default:
			return {};
	}
}

asio::const_buffer GsNetwork::getBuffer(mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)  ///< Extract payload omitting the address that prepends it
{
	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_UDP4: {
			return {aMavlinkMavGsNetwork.payload + 4, static_cast<std::size_t>(aMavlinkMavGsNetwork.payload_len) - 4};
		}

		case MAV_GS_NETWORK_TRANSPORT_TCP6:
		case MAV_GS_NETWORK_TRANSPORT_UDP6: {
			return {aMavlinkMavGsNetwork.payload + 16, static_cast<std::size_t>(aMavlinkMavGsNetwork.payload_len) - 16};
		}

		default:
			return {};
	}

}

void GsNetwork::processConnect(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	asio::error_code err;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
			Sock::Api::getInstance().connect({addr, aMavlinkMavGsNetwork.remote_port}, aMavlinkMavGsNetwork.host_port,
				err);

			break;

		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			Sock::Api::getInstance().connect({addr, aMavlinkMavGsNetwork.remote_port}, aMavlinkMavGsNetwork.host_port,
				err, asio::ip::tcp::v6());

			break;

	}

	aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_FAIL;

	if (asio::error::already_connected == err || !err) {
		ESP_LOGD(Mav::kDebugTag, "GsNetwork::processConnect()");
		aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_SUCCESS;
	} else {
		ESP_LOGD(Mav::kDebugTag, "GsNetwork::processConnect(): fail");
	}
}

void GsNetwork::processDisconnect(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	asio::error_code err;
	Sock::Api::getInstance().disconnect({addr, aMavlinkMavGsNetwork.remote_port}, aMavlinkMavGsNetwork.host_port, err);
	aMavlinkMavGsNetwork.ack = err ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	// Already disconnected
	if (asio::error::not_connected == err) {
		aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_SUCCESS;
	}
}

void GsNetwork::processOpen(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	asio::error_code errorCode;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
			Sock::Api::getInstance().openTcp(aMavlinkMavGsNetwork.host_port, errorCode);

			break;

		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			Sock::Api::getInstance().openTcp(aMavlinkMavGsNetwork.host_port, errorCode, asio::ip::tcp::v6());

			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP4:
			Sock::Api::getInstance().openUdp(aMavlinkMavGsNetwork.host_port, errorCode);

			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP6:
			Sock::Api::getInstance().openUdp(aMavlinkMavGsNetwork.host_port, errorCode, asio::ip::udp::v6());

			break;
	}
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	if (asio::error::already_open == errorCode) {
		aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_SUCCESS;
	}

	if (MAV_GS_NETWORK_ACK_SUCCESS == aMavlinkMavGsNetwork.ack && Bdg::RoutingRules::checkInstance()) {
		if (Ut::Al::in(aMavlinkMavGsNetwork.transport, MAV_GS_NETWORK_TRANSPORT_TCP4,
			MAV_GS_NETWORK_TRANSPORT_TCP6))
		{
			Bdg::RoutingRules::getInstance().addStatic(Bdg::TcpPort{aMavlinkMavGsNetwork.host_port, {}},
				Bdg::NamedEndpoint::MavlinkIpPack, Bdg::NamedEndpoint::MavlinkIpPackForwarded);
		} else {
			Bdg::RoutingRules::getInstance().addStatic(Bdg::UdpPort{aMavlinkMavGsNetwork.host_port},
				Bdg::NamedEndpoint::MavlinkIpPack, Bdg::NamedEndpoint::MavlinkIpPackForwarded);
		}
	}
}

void GsNetwork::processClose(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	asio::error_code errorCode;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			Sock::Api::getInstance().closeTcp(aMavlinkMavGsNetwork.host_port, errorCode);

			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP4:
		case MAV_GS_NETWORK_TRANSPORT_UDP6:
			Sock::Api::getInstance().closeUdp(aMavlinkMavGsNetwork.host_port, errorCode);

			break;
	}
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	if (asio::error::not_found == errorCode) {
		aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_SUCCESS;
	}

	if (MAV_GS_NETWORK_ACK_SUCCESS == aMavlinkMavGsNetwork.ack && Bdg::RoutingRules::checkInstance()) {
		if (Ut::Al::in(aMavlinkMavGsNetwork.transport, MAV_GS_NETWORK_TRANSPORT_TCP4,
			MAV_GS_NETWORK_TRANSPORT_TCP6))
		{
			Bdg::RoutingRules::getInstance().remove(Bdg::TcpPort{aMavlinkMavGsNetwork.host_port, {}},
				Bdg::NamedEndpoint::MavlinkIpPack);
		} else {
			Bdg::RoutingRules::getInstance().remove(Bdg::UdpPort{aMavlinkMavGsNetwork.host_port},
				Bdg::NamedEndpoint::MavlinkIpPack);
		}
	}
}

void GsNetwork::processSend(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	asio::error_code errorCode;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			Sock::Api::getInstance().sendTo(asio::ip::tcp::endpoint{addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port, getBuffer(aMavlinkMavGsNetwork), errorCode);

			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP4:
			Sock::Api::getInstance().sendTo(asio::ip::udp::endpoint{addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port, getBuffer(aMavlinkMavGsNetwork), errorCode);

			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP6:
			Sock::Api::getInstance().sendTo(asio::ip::udp::endpoint{addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port, getBuffer(aMavlinkMavGsNetwork), errorCode, asio::ip::udp::v6());

			break;
	}
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;
}

void GsNetwork::processReceived(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
}

void GsNetwork::onReceive(Bdg::OnReceiveCtx aCtx)
{
	ESP_LOGV(Mav::kDebugTag, "GsNetwork::onReceive()");
	asio::ip::tcp::endpoint tcpEndpoint{};
	std::uint16_t localPort = 0;
	aCtx.endpointVariant.match(
		[&tcpEndpoint, &localPort](const Bdg::TcpEndpoint &aEndpoint)
		{
			tcpEndpoint = std::get<0>(aEndpoint);
			localPort = std::get<1>(aEndpoint);
		},
		[](...){} );
	mavlink_mav_gs_network_t mavlinkMavGsNetwork;
	mavlink_message_t mavlinkMessage;

	// Encode address-related info and payload into the message
	mavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE;
	mavlinkMavGsNetwork.command = MAV_GS_NETWORK_COMMAND_PROCESS_RECEIVED;
	auto nProcessed = initMavlinkMavGsNetwork(mavlinkMavGsNetwork, tcpEndpoint, localPort, aCtx.buffer);
	mavlink_msg_mav_gs_network_encode(Globals::getSysId(), Globals::getCompId(), &mavlinkMessage, &mavlinkMavGsNetwork);

	// Pack the message into a buffer
	const auto kMaxMessageLen = mavGsNetworkGetMaxMessageLength(mavlinkMavGsNetwork.payload_len);
	auto nPacked = Marshalling::push(mavlinkMessage, {resp.buffer, kMaxMessageLen});
	aCtx.forwardCb({{resp.buffer, nPacked}, [](Bdg::RespondCtx){}});
	aCtx.buffer.slice(nProcessed);
}

void GsNetwork::onTcpEvent(typename Sub::Rout::OnTcpEvent::Arg<0> aTcpEvent)
{
	Bdg::Receiver::notifyAsAsync({Bdg::NamedEndpoint::Mavlink,
		[this, aTcpEvent]()
		{
			ESP_LOGV(Mav::kDebugTag, "GsNetwork::onTcpEvent()");
			mavlink_mav_gs_network_t mavlinkMavGsNetwork;
			aTcpEvent.match(
				[&mavlinkMavGsNetwork](const Sub::Rout::TcpConnected &a) {
					initMavlinkMavGsNetwork(mavlinkMavGsNetwork, a.remoteEndpoint, a.localPort, asio::const_buffer(nullptr, 0));
					mavlinkMavGsNetwork.command = MAV_GS_NETWORK_COMMAND_PROCESS_CONNECTED;
				},
				[&mavlinkMavGsNetwork](const Sub::Rout::TcpDisconnected &a) {
					initMavlinkMavGsNetwork(mavlinkMavGsNetwork, a.remoteEndpoint, a.localPort, asio::const_buffer(nullptr, 0));
					mavlinkMavGsNetwork.command = MAV_GS_NETWORK_COMMAND_PROCESS_CONNECTION_ABORTED;
				}
			);
			mavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE;
			mavlink_message_t mavlinkMessage;
			mavlink_msg_mav_gs_network_encode(Globals::getSysId(), Globals::getCompId(), &mavlinkMessage, &mavlinkMavGsNetwork);
			const std::size_t nProcessed = Marshalling::push(mavlinkMessage, resp.buffer);

			return Ut::Cont::ConstBuffer(resp.buffer, nProcessed);
		},
		[](Bdg::RespondCtx){}});
}
