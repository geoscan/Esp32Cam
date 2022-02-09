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
	mavlink_msg_mav_gs_network_decode(&aMavlinkMessage, &mavlinkMavGsNetwork);

	if (mavlinkMavGsNetwork.ack != MAV_GS_NETWORK_ACK_NONE) {  // Response message
		return Ret::NoResponse;
	}

	switch (mavlinkMavGsNetwork.command) {  // Command message
		case MAV_GS_NETWORK_COMMAND_CONNECT:
			return processConnect(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_DISCONNECT:
			return processDisconnect(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_SEND:
			return processSend(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_OPEN:
			return processSend(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_CLOSE:
			return processClose(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_PROCESS_RECEIVED:
			return processReceived(aMavlinkMessage, mavlinkMavGsNetwork);

		default:
			return Ret::Ignored;
	}
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

Microservice::Ret GsNetwork::processConnect(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	auto res = aMavlinkMavGsNetwork.ack == MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE ? Ret::NoResponse : Ret::Response;
	auto errorCode = Sock::Api::getInstance().connect({addr, aMavlinkMavGsNetwork.remote_port},
		aMavlinkMavGsNetwork.host_port);
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	return res;
}

Microservice::Ret GsNetwork::processDisconnect(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	auto res = aMavlinkMavGsNetwork.ack == MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE ? Ret::NoResponse : Ret::Response;
	auto errorCode = Sock::Api::getInstance().disconnect({addr, aMavlinkMavGsNetwork.remote_port},
		aMavlinkMavGsNetwork.host_port);
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	return res;
}

Microservice::Ret GsNetwork::processOpen(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	auto res = aMavlinkMavGsNetwork.ack == MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE ? Ret::NoResponse : Ret::Response;
	asio::error_code errorCode;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			errorCode = Sock::Api::getInstance().open<asio::ip::tcp>({addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port);
			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP4:
		case MAV_GS_NETWORK_TRANSPORT_UDP6:
			errorCode = Sock::Api::getInstance().open<asio::ip::udp>({addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port);
			break;
	}
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	return res;
}

Microservice::Ret GsNetwork::processClose(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	auto res = aMavlinkMavGsNetwork.ack == MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE ? Ret::NoResponse : Ret::Response;
	asio::error_code errorCode;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			errorCode = Sock::Api::getInstance().close<asio::ip::tcp>({addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port);
			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP4:
		case MAV_GS_NETWORK_TRANSPORT_UDP6:
			errorCode = Sock::Api::getInstance().close<asio::ip::udp>({addr, aMavlinkMavGsNetwork.remote_port},
				aMavlinkMavGsNetwork.host_port);
			break;
	}
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	return res;
}

Microservice::Ret GsNetwork::processSend(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	auto addr = getAddress(aMavlinkMavGsNetwork);
	auto res = aMavlinkMavGsNetwork.ack == MAV_GS_NETWORK_ACK_NONE_HOLD_RESPONSE ? Ret::NoResponse : Ret::Response;
	asio::error_code errorCode;

	switch (aMavlinkMavGsNetwork.transport) {
		case MAV_GS_NETWORK_TRANSPORT_TCP4:
		case MAV_GS_NETWORK_TRANSPORT_TCP6:
			errorCode = Sock::Api::getInstance().sendTo<asio::ip::tcp>({addr, aMavlinkMavGsNetwork.remote_port},
				getBuffer(aMavlinkMavGsNetwork),
				aMavlinkMavGsNetwork.host_port);
			break;

		case MAV_GS_NETWORK_TRANSPORT_UDP4:
		case MAV_GS_NETWORK_TRANSPORT_UDP6:
			errorCode = Sock::Api::getInstance().sendTo<asio::ip::udp>({addr, aMavlinkMavGsNetwork.remote_port},
				getBuffer(aMavlinkMavGsNetwork),
				aMavlinkMavGsNetwork.host_port);
			break;
	}
	aMavlinkMavGsNetwork.ack = errorCode ? MAV_GS_NETWORK_ACK_FAIL : MAV_GS_NETWORK_ACK_SUCCESS;

	return res;
}

Microservice::Ret GsNetwork::processReceived(mavlink_message_t &aMavlinkMessage,
	mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	return Ret::Ignored;
}

