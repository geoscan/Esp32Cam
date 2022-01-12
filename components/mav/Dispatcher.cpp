//
// Dispatcher.cpp
//
// Created on: Dec 21, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sub/Subscription.hpp"
#include "Mavlink.hpp"
#include "Marshalling.hpp"
#include "Unmarshalling.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Dispatcher.hpp"

Mav::Dispatcher::Dispatcher():
	key{{&Dispatcher::onMavlinkUartReceived, this}, {&Dispatcher::onMavlinkUdpReceived, this}},
	micAggregate{}
{
}

Mav::Microservice::Ret Mav::Dispatcher::process(Utility::ConstBuffer aBuffer)
{
	auto ret = Microservice::Ret::Ignored;
	unmarshalling.push(aBuffer);  // parse incoming message, check whether it is a mavlink

	if (unmarshalling.size()) {
		ESP_LOGI("mav dispatcher", "message received");
		auto &message = unmarshalling.front();
		ret = micAggregate.process(message);

		if (ret == Microservice::Ret::Response) {
			marshalling.push(message);
		}
		unmarshalling.pop();
	}

	return ret;
}

Sub::ProcessReceivedResult Mav::Dispatcher::onMavlinkUartReceived(Sub::Message &aMessage)
{
	// TODO: consider sysid, compid checking, preamble parsing, or maybe other means of optimizing the forwarding to reduce time expenses.
	switch (process(aMessage.payload)) {
		case Microservice::Ret::Ignored:  // forward the message to UDP interface
			Sub::Key::MavlinkUdpSend::notify(aMessage);
			break;

		case Microservice::Ret::Response:  // send response back
			aMessage.payload = Utility::Buffer(&marshalling.back(), sizeof(marshalling.back()));
			Sub::Key::MavlinkUartSend::notify(aMessage);
			marshalling.pop();
			break;

		default:  // Message has been processed by some Microservice instance, no actions required
			break;
	};

	return {};
}

Sub::ProcessReceivedResult Mav::Dispatcher::onMavlinkUdpReceived(Sub::Message &aMessage)
{
	Sub::Key::MavlinkUartSend::notify(aMessage);  // forward the message to UART interface

	return {};
}
