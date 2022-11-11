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
#include "wifi_uart_bridge/Receiver.hpp"
#include "mav/mav.hpp"
#include "utility/LogSection.hpp"

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Mav::Dispatcher, onReceive, 0)
GS_UTILITY_LOGD_METHOD_SET_ENABLED(Mav::Dispatcher, process, 1)
GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Mav::Dispatcher, "async", 1);

Mav::Dispatcher::Dispatcher() :
	Bdg::Receiver{Bdg::NamedEndpoint::Mavlink, "Mavlink dispatcher"},
	key{{&Dispatcher::onMavlinkReceived, this}},
	micAggregate{*this}
{
}

void Mav::Dispatcher::onSubscription(const mavlink_message_t &aMavlinkMessage)
{
	// Warn. Do not replace iteration w/ RR's Key<...>::notify(), because locking order matters
	Bdg::Receiver::notifyAs({Bdg::NamedEndpoint::Mavlink,
		{resp.buffer, Marshalling::push(aMavlinkMessage, resp.buffer)}, [](Bdg::RespondCtx){}});
}

/// \brief Pushes a lightweight buffer producer into the notification chain.
///
/// \details When the current sender's turn comes, it will get notified, and
/// will be expected to produce a buffer. No synchronization is required, as
/// the message pipeline will have already been locked by that moment.
void Mav::Dispatcher::onSubscription(DelayedSendAsyncCtx delayedSendAsyncCtx)
{
	Bdg::Receiver::notifyAsAsync({
		Bdg::NamedEndpoint::Mavlink,
		[this, delayedSendAsyncCtx]()
		{
			mavlink_message_t mavlinkMessage{};
			delayedSendAsyncCtx.delayedSendAsyncVariant.match(
				[&mavlinkMessage](const mavlink_camera_tracking_image_status_t &aPack)
				{
					GS_UTILITY_LOGV_CLASS_ASPECT(Mav::kDebugTag, Dispatcher, "async",
						"Packing mavlink_camera_tracking_image_status_t");
					mavlink_msg_camera_tracking_image_status_encode(0, 0, &mavlinkMessage, &aPack);
				});
			mavlinkMessage.sysid = delayedSendAsyncCtx.sysId;
			mavlinkMessage.compid = delayedSendAsyncCtx.compId;
			const auto nPacked = Marshalling::push(mavlinkMessage, resp.buffer);
			GS_UTILITY_LOGV_CLASS_ASPECT(Mav::kDebugTag, Dispatcher, "async",
				"packed %d bytes", nPacked);
			Ut::Cont::ConstBuffer ret = {resp.buffer, nPacked};

			return ret;
		},
		[](Bdg::RespondCtx) {}
	});
}

Mav::Microservice::Ret Mav::Dispatcher::process(Ut::Cont::ConstBuffer aBuffer, int &anProcessed)
{
	auto ret = Microservice::Ret::Ignored;
	anProcessed = unmarshalling.push(aBuffer);  // parse incoming message, check whether it is a mavlink

	if (unmarshalling.size()) {
		auto &message = unmarshalling.front();

		resp.size = 0;
		ret = micAggregate.process(message,
			[this](mavlink_message_t &aMsg) mutable {
				std::size_t inc = Marshalling::push(aMsg,
					Ut::Cont::Buffer{resp.buffer, sizeof(resp.buffer)}.asSlice(resp.size));
				GS_UTILITY_LOGD_METHOD(Mav::kDebugTag, Dispatcher, process, "(on response callback)"
					"pushed %d bytes into response buffer", inc);
				resp.size += inc;
			});

		unmarshalling.pop();
	}

	return ret;
}

Sub::Rout::OnMavlinkReceived::Ret Mav::Dispatcher::onMavlinkReceived(Sub::Rout::OnMavlinkReceived::Arg<0> aMessage)
{
	// TODO: consider sysid, compid checking, preamble parsing, or maybe other means of optimizing the forwarding to reduce time expenses.
	Sub::Rout::Response response{Sub::Rout::Response::Type::Ignored};

	switch (process(Ut::Cont::toBuffer<const void>(aMessage), response.nProcessed)) {
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

void Mav::Dispatcher::onReceive(Bdg::OnReceiveCtx aCtx)
{
	int nprocessed = 0;
	switch (process(aCtx.buffer, nprocessed)) {
		case Microservice::Ret::Ignored:
			aCtx.forwardCb({aCtx.buffer.asSlice(0, nprocessed), aCtx.respondCb});

			break;

		case Microservice::Ret::NoResponse:
			break;

		case Microservice::Ret::Response:
			GS_UTILITY_LOGV_METHOD(Mav::kDebugTag, Dispatcher, onReceive, "Formed a response of %d bytes. Sending back",
				resp.size);
			aCtx.respondCb({{static_cast<void *>(resp.buffer), resp.size}});

			break;
	}

	GS_UTILITY_LOGV_METHOD(Mav::kDebugTag, Dispatcher, onReceive, "processed %d bytes", nprocessed);
	aCtx.buffer.slice(nprocessed);  // Change the size of the buffer, so `Dispatcher` will be re-notified
}
