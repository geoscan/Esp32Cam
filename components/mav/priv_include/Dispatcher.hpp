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

#include "Globals.hpp"
#include "sub/Subscription.hpp"
#include "sub/Rout.hpp"
#include "Microservice.hpp"
#include "Microservice/Aggregate.hpp"
#include "Microservice/ApVer.hpp"
#include "DelayedSend.hpp"
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
}

class Dispatcher : public DelayedSendHandle {
public:
	Dispatcher();

	void onSubscription(const mavlink_message_t &) override;

protected:
	Mav::Microservice::Ret process(Utility::ConstBuffer aMessage, int &anProcessed);

private:
	Sub::Rout::OnMavlinkReceived::Ret onMavlinkReceived(Sub::Rout::OnMavlinkReceived::Arg<0>);
	Sub::Rout::Payload respAsPayload();

private:
	struct {
		Sub::Rout::OnMavlinkReceived onMavlinkReceived;
	} key;

	Unmarshalling unmarshalling;
	Mav::Mic::Aggregate<Mic::GsNetwork, Mic::Camera, Mic::ApVer> micAggregate;

	struct {
		std::uint8_t buffer[sizeof(mavlink_message_t)];
		typename Sub::Rout::PayloadLock::element_type::mutex_type mutex;
		std::size_t size = 0;
	} resp;
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_DISPATCHER_HPP
