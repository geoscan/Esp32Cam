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
#include "Microservice/Camera.hpp"
#include "Dispatcher.hpp"
#include "mav/mav.hpp"

Mav::Dispatcher::Dispatcher() :
	Bdg::Receiver{Bdg::NamedEndpoint::Mavlink},
	key{{&Dispatcher::onMavlinkReceived, this}},
	micAggregate{*this}
{
}

void Mav::Dispatcher::onSubscription(const mavlink_message_t &aMavlinkMessage)
{
	// Warn. Do not replace iteration w/ RR's Key<...>::notify(), because locking order matters
	for (auto &cb : Sub::Rout::OnReceived::getIterators()) {  // Iterate over subscribers in a thread-safe way
		std::lock_guard<std::mutex> lock{resp.mutex};  // Lock response buffer
		(void)lock;
		resp.size = Marshalling::push(aMavlinkMessage, resp.buffer);
		cb({Sub::Rout::Mavlink{respAsPayload()}});
	}
}

Mav::Microservice::Ret Mav::Dispatcher::process(Utility::ConstBuffer aBuffer, int &anProcessed)
{
	auto ret = Microservice::Ret::Ignored;
	anProcessed = unmarshalling.push(aBuffer);  // parse incoming message, check whether it is a mavlink

	if (unmarshalling.size()) {
		auto &message = unmarshalling.front();

		resp.size = 0;
		ret = micAggregate.process(message,
			[this](mavlink_message_t &aMsg) mutable {
				ESP_LOGD(Mav::kDebugTag, "Dispatcher::process::lambda (on response)");
				resp.size += Marshalling::push(aMsg, Utility::Buffer{resp.buffer, sizeof(resp.buffer)}.slice(resp.size));
			});

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
			ESP_LOGD(Mav::kDebugTag, "Dispatcher::onMavlinkReceived: sending response, size %d", resp.size);
			response.payloadLock = Sub::Rout::PayloadLock{new Sub::Rout::PayloadLock::element_type{resp.mutex}};
			response.payload = respAsPayload();

			break;

		default:  // Message has been processed by some Microservice instance, no actions required
			break;
	};

	ESP_LOGV(Mav::kDebugTag, "Dispatcher::process(): processed %d bytes", response.nProcessed);

	return response;
}

Sub::Rout::Payload Mav::Dispatcher::respAsPayload()
{
	return Sub::Rout::Payload{resp.buffer, resp.size};
}

void Mav::Dispatcher::onReceive(const Bdg::EndpointVariant &aSender, Utility::ConstBuffer aBuffer,
	Bdg::RespondCb aRespondCb, Bdg::ForwardCb aForwardCb)
{
	(void)aSender;
	(void)aBuffer;
	(void)aRespondCb;
	(void)aForwardCb;
}
