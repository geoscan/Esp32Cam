//
// Dispatcher.cpp
//
// Created on: Dec 21, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "sub/Subscription.hpp"
#include "Mavlink.hpp"
#include "Marshalling.hpp"
#include "Unmarshalling.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Dispatcher.hpp"
#include "mav/mav.hpp"

Mav::Dispatcher::Dispatcher():
	key{{&Dispatcher::onMavlinkReceived, this}},
	micAggregate{}
{
}

Mav::Microservice::Ret Mav::Dispatcher::process(Utility::ConstBuffer aBuffer, int &anProcessed)
{
	auto ret = Microservice::Ret::Ignored;
	anProcessed = unmarshalling.push(aBuffer);  // parse incoming message, check whether it is a mavlink

	if (unmarshalling.size()) {
		auto &message = unmarshalling.front();
		ret = micAggregate.process(message);

		if (ret == Microservice::Ret::Response) {
			resp.size = Marshalling::push(message, resp.buffer);
		}
		unmarshalling.pop();
	}

	return ret;
}


Sub::Rout::OnMavlinkReceived::Ret Mav::Dispatcher::onMavlinkReceived(Sub::Rout::OnMavlinkReceived::Arg<0> aMessage)
{
	// TODO: consider sysid, compid checking, preamble parsing, or maybe other means of optimizing the forwarding to reduce time expenses.
	Sub::Rout::Response response{Sub::Rout::Response::Type::Ignored};

	switch (process(Utility::toBuffer<const void>(aMessage), response.nProcessed)) {
		case Microservice::Ret::Ignored:  // forward the message to UDP interface
			response.setType(Sub::Rout::Response::Type::Ignored);

			break;

		case Microservice::Ret::NoResponse:
			response.setType(Sub::Rout::Response::Type::Consumed);

			break;

		case Microservice::Ret::Response:  // send response back
			response.payloadLock = Sub::Rout::PayloadLock{new Sub::Rout::PayloadLock::element_type{resp.mutex}};
			response.payload = Sub::Rout::Payload{&resp.buffer, resp.size};

			break;

		default:  // Message has been processed by some Microservice instance, no actions required
			break;
	};

	ESP_LOGV(Mav::kDebugTag, "Dispatcher::process(): processed %d bytes", response.nProcessed);

	return response;
}
