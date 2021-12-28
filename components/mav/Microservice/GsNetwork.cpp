//
// GsNetwork.cpp
//
// Created on: Dec 22, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include "Mavlink.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Globals.hpp"
#include "utility/Subscription.hpp"
#include <algorithm>
#include <utility/Algorithm.hpp>

using namespace Mav::Mic;

Mav::Microservice::Ret GsNetwork::process(mavlink_message_t &aMavlinkMessage)
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
		case MAV_GS_NETWORK_COMMAND_DISCONNECT:
			return processConnectDisconnect(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_SEND:
			return processSend(aMavlinkMessage, mavlinkMavGsNetwork);

		case MAV_GS_NETWORK_COMMAND_PROCESS_RECEIVED:
			return processReceived(aMavlinkMessage, mavlinkMavGsNetwork);

		default:
			return Ret::Ignored;
	}
}

Mav::Microservice::Ret GsNetwork::processConnectDisconnect(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{

	if (aMavlinkMavGsNetwork.transport == MAV_GS_NETWORK_TRANSPORT_TCP) {  // Connect command can only be evaluated for TCP transport
		aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_FAIL;
	} else {
		Utility::Subscription::IpConnect ipConnect;

		std::copy(aMavlinkMavGsNetwork.dest_ip4, aMavlinkMavGsNetwork.dest_ip4 + 4, ipConnect.address);
		ipConnect.port = aMavlinkMavGsNetwork.dest_port;
		ipConnect.hostPort = aMavlinkMavGsNetwork.src_port;
		ipConnect.connect = (aMavlinkMavGsNetwork.command == MAV_GS_NETWORK_COMMAND_CONNECT);
		aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_FAIL;

		for (auto &ipConnectService : Utility::Subscription::Key::IpConnect::getIterators()) {
			auto result = ipConnectService(ipConnect);

			if (result.resultCode == Utility::Subscription::ResultCode::Success) {
				aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_SUCCESS;
				break;
			}
		}
	}

	mavlink_msg_mav_gs_network_encode(Mav::Globals::getSysId(), Mav::Globals::getCompId(), &aMavlinkMessage, &aMavlinkMavGsNetwork);
	std::memset(aMavlinkMavGsNetwork.payload, 0, sizeof(aMavlinkMavGsNetwork.payload));

	return Ret::Response;
}

Mav::Microservice::Ret GsNetwork::processSend(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	Utility::Subscription::IpDestMessage ipDestMessage;

	ipDestMessage.payload = Utility::ConstBuffer{aMavlinkMavGsNetwork.payload, aMavlinkMavGsNetwork.length};
	ipDestMessage.port = aMavlinkMavGsNetwork.dest_port;
	ipDestMessage.hostPort = aMavlinkMavGsNetwork.src_port;
	std::copy(aMavlinkMavGsNetwork.dest_ip4, aMavlinkMavGsNetwork.dest_ip4 + 4, ipDestMessage.address);
	ipDestMessage.transport = (aMavlinkMavGsNetwork.transport == MAV_GS_NETWORK_TRANSPORT_TCP) ?
		Utility::Subscription::IpTransport::Tcp : Utility::Subscription::IpTransport::Udp;
	aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_FAIL;

	for (auto &ipSendService : Utility::Subscription::Key::IpSend::getIterators()) {
		auto result = ipSendService(ipDestMessage);

		if (result.resultCode == Utility::Subscription::ResultCode::Success) {
			aMavlinkMavGsNetwork.ack = MAV_GS_NETWORK_ACK_SUCCESS;
			break;
		}
	}

	mavlink_msg_mav_gs_network_encode(Mav::Globals::getSysId(), Mav::Globals::getCompId(), &aMavlinkMessage, &aMavlinkMavGsNetwork);
	std::memset(aMavlinkMavGsNetwork.payload, 0, sizeof(aMavlinkMavGsNetwork.payload));

	return Ret::Response;
}

Mav::Microservice::Ret GsNetwork::processReceived(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	return Ret::Ignored;
}
