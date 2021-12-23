//
// GsNetwork.cpp
//
// Created on: Dec 22, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include "Mavlink.hpp"
#include "Microservice/GsNetwork.hpp"
#include "utility/Subscription.hpp"
#include <algorithm>

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
//	using namespace Utility::Subscription;

//	IpEndpointCommand ipEndpointCommand;
//	ipEndpointCommand.command = aMavlinkMavGsNetwork.command == MAV_GS_NETWORK_COMMAND_CONNECT ? IpEndpointCommand::Connect : IpEndpointCommand::Disconnect;
//	ipEndpointCommand.port = aMavlinkMavGsNetwork.src_port;
//	ipEndpointCommand.endpoint.port = aMavlinkMavGsNetwork.dest_port;
//	std::copy_n(aMavlinkMavGsNetwork.dest_ip4, sizeof(aMavlinkMavGsNetwork.dest_ip4), ipEndpointCommand.ipv4);

//	unsigned result = IpEndpointResult::Unprocessed;

//	for (auto &callable : Key::ProcessIpEndpointCommand::getIterators()) {
//		if ((result = callable(ipEndpointCommand)) != IpEndpointResult::Unprocessed) {
//			break;
//		}
//	}

//	aMavlinkMavGsNetwork.ack = (result == IpEndpointResult::Success) ? MAV_GS_NETWORK_ACK_SUCCESS :
//		MAV_GS_NETWORK_ACK_FAIL;

	return Ret::Ignored;
}

Mav::Microservice::Ret GsNetwork::processSend(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	return Ret::Ignored;
}

Mav::Microservice::Ret GsNetwork::processReceived(mavlink_message_t &aMavlinkMessage, mavlink_mav_gs_network_t &aMavlinkMavGsNetwork)
{
	return Ret::Ignored;
}
