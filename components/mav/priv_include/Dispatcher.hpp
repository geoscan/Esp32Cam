//
// Dispatcher.hpp
//
// Created on: Dec 21, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Accepts messages passed through a callback mechanism, and passes those to
// encapsulated microservices.

#ifndef MAV_PRIV_INCLUDE_DISPATCHER_HPP
#define MAV_PRIV_INCLUDE_DISPATCHER_HPP

#include "DelayedSend.hpp"
#include "Globals.hpp"
#include "Microservice.hpp"
#include "Microservice/Aggregate.hpp"
#include "Microservice/ApVer.hpp"
#include "Microservice/BftFtpClient.hpp"
#include "Microservice/BufferedFileTransfer.hpp"
#include "sub/Rout.hpp"
#include "sub/Subscription.hpp"
#include "wifi_uart_bridge/Receiver.hpp"
#include <list>
#include <memory>

struct __mavlink_message;
using mavlink_message_t = __mavlink_message;

namespace Mav {

class Marshalling;
class Unmarshalling;

namespace Mic {
class GsNetwork;
class Camera;
class Tracking;
class Wifi;
}

class Dispatcher : public DelayedSendHandle, public Bdg::Receiver {
public:
	Dispatcher();
	void onSubscription(const mavlink_message_t &) override;
	void onSubscription(DelayedSendAsyncCtx delayedSendAsyncCtx) override;
protected:
	Mav::Microservice::Ret process(Ut::Cont::ConstBuffer aMessage, int &anProcessed);
private:
	Sub::Rout::Payload respAsPayload();
	void onReceive(Bdg::OnReceiveCtx) override;
private:

	Unmarshalling unmarshalling;
	Mav::Mic::Aggregate<Mic::Wifi, Mic::GsNetwork, Mic::Camera, Mic::ApVer, Mic::Tracking, Mic::BftFtpClient,
		Mic::BufferedFileTransfer> micAggregate;

	struct {
		std::uint8_t buffer[sizeof(mavlink_message_t)];
		typename Sub::Rout::PayloadLock::element_type::mutex_type mutex;
		std::size_t size = 0;
	} resp;
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_DISPATCHER_HPP
