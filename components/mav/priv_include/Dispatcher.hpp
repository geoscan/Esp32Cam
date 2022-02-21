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

#include "sub/Subscription.hpp"
#include "sub/Rout.hpp"
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
	Sub::Rout::OnMavlinkReceived::Ret onMavlinkReceived(Sub::Rout::OnMavlinkReceived::Arg<0>);

private:
	struct {
		Sub::Rout::OnMavlinkReceived onMavlinkReceived;
	} key;

	Marshalling marshalling;
	Unmarshalling unmarshalling;
	std::size_t marshallingSize = 0;
	Mav::Mic::Aggregate<Mic::GsNetwork> micAggregate;
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_DISPATCHER_HPP
