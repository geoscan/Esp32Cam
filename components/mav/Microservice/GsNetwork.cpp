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
#include <algorithm>
#include <utility/Algorithm.hpp>
#include <memory>
#include "mav/mav.hpp"

using namespace Mav;
using namespace Mav::Mic;
using namespace asio::ip;

GsNetwork::GsNetwork() :
	Bdg::Receiver{Bdg::NamedEndpoint::MavlinkIpPack},
	key{
		{&GsNetwork::packForward<asio::ip::tcp>, this},
		{&GsNetwork::packForward<asio::ip::udp>, this},
		{&GsNetwork::packTcpEvent, this},
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

	ESP_LOGD(Mav::kDebugTag, "GsNetwork:process: mavlink_mav_gs_network_t: host_port %d remote_port: %d "
		"command %d ack %d transport %d payload_len %d, payload: %*.*s", mavlinkMavGsNetwork.host_port,
		mavlinkMavGsNetwork.remote_port, mavlinkMavGsNetwork.command, mavlinkMavGsNetwork.ack,
		mavlinkMavGsNetwork.transport, mavlinkMavGsNetwork.payload_len, mavlinkMavGsNetwork.payload_len,
		mavlinkMavGsNetwork.payload_len, mavlinkMavGsNetwork.payload);

	switch (mavlinkMavGsNetwork.ack) {
		case MAV_GS_NETWORK_ACK_NONE:
			ret = Ret::Response;
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process: response required");

			break;

		case MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE:
			ret = Ret::NoResponse;
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process: No response required");

			break;

		default:  // Non-request message
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process: Non-request message");
			return Ret::NoResponse;
	}

	switch (mavlinkMavGsNetwork.command) {  // Command message
		case MAV_GS_NETWORK_COMMAND_CONNECT:
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process CONNECT");
			processConnect(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_DISCONNECT:
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process DISCONNECT");
			processDisconnect(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_SEND:
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process SEND");
			processSend(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_OPEN:
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process OPEN");
			processOpen(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_CLOSE:
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process CLOSE");
			processClose(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_PROCESS_RECEIVED:
			ESP_LOGD(Mav::kDebugTag, "GsNetwork:process COMMAND_PROCESS_RECEIVED");
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

typename Sub::Rout::MavlinkPackTcpEvent::Ret GsNetwork::packTcpEvent(
	typename Sub::Rout::MavlinkPackTcpEvent::Arg<0> arg)
{
	mavlink_mav_gs_network_t mavlinkMavGsNetwork;

	auto visitor = mapbox::util::make_visitor(
		[&](const Sub::Rout::TcpConnected &a) {
			initMavlinkMavGsNetwork(mavlinkMavGsNetwork, a.remoteEndpoint, a.localPort, asio::const_buffer(nullptr, 0));
			mavlinkMavGsNetwork.command = MAV_GS_NETWORK_COMMAND_PROCESS_CONNECTED;
		},
		[&](const Sub::Rout::TcpDisconnected &a) {
			initMavlinkMavGsNetwork(mavlinkMavGsNetwork, a.remoteEndpoint, a.localPort, asio::const_buffer(nullptr, 0));
			mavlinkMavGsNetwork.command = MAV_GS_NETWORK_COMMAND_PROCESS_CONNECTION_ABORTED;
		}
	);
	mapbox::util::apply_visitor(visitor, arg);
	mavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE;

	return respPackLock(mavlinkMavGsNetwork);  // Pack the message into `resp` structure, set `resp.mutex	` so it will be safely stored there until the response is processed and passed down the communication chain
}

///
/// \brief Pack a mesasge into `Rout::Response` and ensure its thread safety w/ `resp.mutex`
///
Sub::Rout::Response GsNetwork::respPackLock(const mavlink_mav_gs_network_t &mavlinkMavGsNetwork)
{
	mavlink_message_t mavlinkMessage;
	mavlink_msg_mav_gs_network_encode(Globals::getSysId(), Globals::getCompId(), &mavlinkMessage, &mavlinkMavGsNetwork);
	Sub::Rout::Response ret{Sub::Rout::Response::Type::Response};

	// Pack the message into a buffer
	ret.payloadLock = Sub::Rout::PayloadLock{new Sub::Rout::PayloadLock::element_type{resp.mutex}};
	auto nPacked = Marshalling::push(mavlinkMessage, {resp.buffer, sizeof(resp.buffer)});
	ret.payload = {resp.buffer, nPacked};
	ret.nProcessed = -1;  // No fragmented processing is used

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

void GsNetwork::onReceive(const Bdg::EndpointVariant &aSender, Bdg::Buffer aBuffer, Bdg::RespondCb aRespondCb,
	Bdg::ForwardCb aForwardCb)
{
	asio::ip::tcp::endpoint tcpEndpoint{};
	std::uint16_t localPort = 0;
	aSender.match(
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
	auto nProcessed = initMavlinkMavGsNetwork(mavlinkMavGsNetwork, tcpEndpoint, localPort, aBuffer);
	mavlink_msg_mav_gs_network_encode(Globals::getSysId(), Globals::getCompId(), &mavlinkMessage, &mavlinkMavGsNetwork);

	// Pack the message into a buffer
	const auto kMaxMessageLen = mavGsNetworkGetMaxMessageLength(mavlinkMavGsNetwork.payload_len);
	auto nPacked = Marshalling::push(mavlinkMessage, {resp.buffer, kMaxMessageLen});
	aForwardCb({resp.buffer, nPacked}, [](Bdg::ExternalBuffer){});

	aBuffer.slice(nProcessed);
}
