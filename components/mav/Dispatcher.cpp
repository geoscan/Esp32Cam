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
	key{{&Dispatcher::onMavlinkReceived, this}},
	micAggregate{}
{
}

Mav::Microservice::Ret Mav::Dispatcher::process(Utility::ConstBuffer aBuffer)
{
	auto ret = Microservice::Ret::Ignored;
	unmarshalling.push(aBuffer);  // parse incoming message, check whether it is a mavlink

	if (unmarshalling.size()) {
		auto &message = unmarshalling.front();
		ret = micAggregate.process(message);

		if (ret == Microservice::Ret::Response) {
			marshalling.push(message);
		}
		unmarshalling.pop();
	}

	return ret;
}


Sub::Rout::OnMavlinkReceived::Ret Mav::Dispatcher::onMavlinkReceived(Sub::Rout::OnMavlinkReceived::Arg<0> aMessage)
{
	// TODO: consider sysid, compid checking, preamble parsing, or maybe other means of optimizing the forwarding to reduce time expenses.
	Sub::Rout::Response response{Sub::Rout::Response::Type::Ignored};

	while (marshalling.size()) {
		marshalling.pop();
	}

	switch (process(Utility::toBuffer<const void>(aMessage))) {
		case Microservice::Ret::Ignored:  // forward the message to UDP interface
			response.setType(Sub::Rout::Response::Type::Ignored);

			break;

		case Microservice::Ret::NoResponse:
			response.setType(Sub::Rout::Response::Type::Consumed);

			break;

		case Microservice::Ret::Response:  // send response back
			response.setType(Sub::Rout::Response::Type::Response);
			response.payload = Sub::Rout::Payload{&marshalling.back(), sizeof(marshalling.back())};

			break;

		default:  // Message has been processed by some Microservice instance, no actions required
			break;
	};

	return response;
}
