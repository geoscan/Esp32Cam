//
// GsNetwork.cpp
//
// Created on: Dec 22, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include "Mavlink.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Globals.hpp"
#include "sub/Subscription.hpp"
#include "sub/Socket.hpp"
#include "socket/Api.hpp"
#include <algorithm>
#include <utility/Algorithm.hpp>

using namespace Mav;
using namespace Mav::Mic;
using namespace Sub::Socket;
using namespace asio::ip;

Microservice::Ret GsNetwork::process(mavlink_message_t &aMavlinkMessage)
{
	if (aMavlinkMessage.msgid != MAVLINK_MSG_ID_MAV_GS_NETWORK) {
		return Ret::Ignored;
	}

	mavlink_mav_gs_network_t mavlinkMavGsNetwork;
	Ret ret;
	mavlink_msg_mav_gs_network_decode(&aMavlinkMessage, &mavlinkMavGsNetwork);

	switch (mavlinkMavGsNetwork.ack) {
		case MAV_GS_NETWORK_ACK_NONE:
			ret = Ret::NoResponse;

			break;

		case MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE:
			ret = Ret::Response;

			break;

		default:  // Non-request message
			return Ret::NoResponse;
	}

	switch (mavlinkMavGsNetwork.command) {  // Command message
		case MAV_GS_NETWORK_COMMAND_CONNECT:
			processConnect(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_DISCONNECT:
			processDisconnect(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_SEND:
			processSend(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_OPEN:
			processSend(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_CLOSE:
			processClose(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		case MAV_GS_NETWORK_COMMAND_PROCESS_RECEIVED:
			processReceived(aMavlinkMessage, mavlinkMavGsNetwork);

			break;

		default:
			return Ret::Ignored;
	}

	return ret;
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

	aMavlinkMavGsNetwork.ack = err ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;
}

void GsNetwork::processDisconnect(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	asio::error_code err;
	Sock::Api::getInstance().disconnect({addr, aMavlinkMavGsNetwork.remote_port}, aMavlinkMavGsNetwork.host_port, err);
	aMavlinkMavGsNetwork.ack = err ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;
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
