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

#include "utility/Subscription.hpp"
#include "Microservice.hpp"
#include "Microservice/Aggregate.hpp"
#include <list>
#include <memory>

struct __mavlink_message;
using mavlink_message_t = __mavlink_message;

namespace Mav {

class Marshalling;
class Unmarshalling;

namespace Mic {
class GsNetwork;
}

class Dispatcher {
public:
	Dispatcher();

protected:
	Mav::Microservice::Ret process(Utility::ConstBuffer aMessage);

private:
	Utility::Subscription::ProcessReceivedResult onMavlinkUartReceived(Utility::Subscription::Message &aMessage);
	Utility::Subscription::ProcessReceivedResult onMavlinkUdpReceived(Utility::Subscription::Message &aMessage);

private:
	struct {
		Utility::Subscription::Key::MavlinkUartReceived fromUart;
		Utility::Subscription::Key::MavlinkUdpReceived fromUdp;
	} key;

	Marshalling marshalling;
	Unmarshalling unmarshalling;
	Mav::Mic::Aggregate<Mic::GsNetwork> micAggregate;
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_DISPATCHER_HPP
