//
// Subscription.hpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// The following is a synchronous implementation of a subscription model
// (Observer / Listener). The necessity for that arises from the fact that
// native ESP's event-handling facilities only offer an asynchronous mechanism.
// As for Mar 03, 2021, the mechanism is applied in 'camera_streamer' module.
//

#ifndef UTILITY_SUBSCRIPTION_HPP
#define UTILITY_SUBSCRIPTION_HPP

#include <memory>
#include <set>

// The following code is not thread-safe. However, you are encouraged to
// implement ad-hoc thread-safety facilities with regard to specifics of your
// task. See 'camera_streamer' module, if you'd like to look at an example.

namespace Utility {
namespace Subscription {

class Sender;

class Subscriber {
public:
	virtual void process(void * = nullptr);

	Subscriber() = default;
	virtual ~Subscriber() = default;
};


class Sender {
protected:
	std::set<Subscriber *> subscribers;

public:
	virtual void addSubscriber(Subscriber &s);

	virtual void removeSubscriber(Subscriber &s);

	virtual void sendToSubscribers(void *ptr = nullptr);

	virtual ~Sender() = default;
};


}  // namespace Subscription

}  // namespace Utility

#endif  // UTILITY_SUBSCRIPTION_HPP
