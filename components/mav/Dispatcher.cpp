//
// Dispatcher.cpp
//
// Created on: Dec 21, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/Subscription.hpp"
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

Mav::Microservice::Ret Mav::Dispatcher::process(Utility::Subscription::Message &aMessage)
{
	auto ret = Microservice::Ret::Ignored;
	unmarshalling.push(aMessage.payload);  // parse incoming message, check whether it is a mavlink

	if (unmarshalling.size()) {
		auto message = unmarshalling.back();
		ret = micAggregate.process(message);

		if (ret == Microservice::Ret::Response) {
			marshalling.push(message);
			unmarshalling.pop();
		}
	}

	return ret;
}

Utility::Subscription::ProcessReceivedResult Mav::Dispatcher::onMavlinkUartReceived(Utility::Subscription::Message &aMessage)
{
	// TODO: consider sysid, compid checking, preamble parsing, or maybe other means of optimizing the forwarding to reduce time expenses.
	switch (process(aMessage)) {
		case Microservice::Ret::Ignored:  // forward the message to UDP interface
			Utility::Subscription::Key::MavlinkUdpSend::notify(aMessage);
			break;

		case Microservice::Ret::Response:  // send response back
			aMessage.payload = marshalling.back().asBuffer();
			Utility::Subscription::Key::MavlinkUartSend::notify(aMessage);
			marshalling.pop();
			break;

		default:  // Message has been processed by some Microservice instance, no actions required
			break;
	};

	return {};
}

Utility::Subscription::ProcessReceivedResult Mav::Dispatcher::onMavlinkUdpReceived(Utility::Subscription::Message &aMessage)
{
	Utility::Subscription::Key::MavlinkUartSend::notify(aMessage);  // forward the message to UART interface

	return {};
}
