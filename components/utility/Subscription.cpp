//
// Subscription.cpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <utility/Subscription.hpp>

using namespace Utility::Subscription;


// ------------ Subscriber ------------ //

void Subscriber::process(void *)
{
}


// ------------ Sender ------------ //

void Sender::addSubscriber(Subscriber &s)
{
	subscribers.insert(&s);
}

void Sender::removeSubscriber(Subscriber &s)
{
	auto it = subscribers.find(&s);
	if (it != subscribers.end()) {
		subscribers.erase(it);
	}
}

void Sender::sendToSubscribers(void *ptr)
{
	for (auto &subscriber : subscribers) {
		subscriber->process(ptr);
	}
}