//
// Receiver.cpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Receiver.hpp"
#include "utility/cont/ListArray.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "utility/comm/RaiiCounter.hpp"
#include "utility/thr/WorkQueue.hpp"
#include "utility/comm/InstanceRegistry.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"

namespace Bdg {

static struct RouteDetails {
	std::mutex mutex;
	unsigned counter;
} sRouteDetails {{}, 0};

constexpr std::chrono::milliseconds kNotifyWait{20};

/// \brief Instantiate a notification chain and wait for it to get finished.
///
void Receiver::notifyAs(const EndpointVariant &aEndpointVariant, Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
{
	ReceiverImpl::Route route{aEndpointVariant};
	route.lock();
	notifyAsAsync(sRouteDetails.counter, aEndpointVariant, aBuffer, std::move(aRespondCb));

	while (!route.checkDone()) {
		Utility::Tim::taskDelay(kNotifyWait);
	}

	route.unlock();
}

Receiver::Receiver(const EndpointVariant &aIdentity, ReceiveCb &&aReceiveCb) :
	receiveCb{aReceiveCb},
	endpointVariant{aIdentity}
{
}

/// \brief Determine which receivers are eligible for being notified upon an incoming message based on `RoutingRules`.
/// Notify each of them. If forwarding is required, push the next notification chain in a work queue.
///
void Receiver::notifyAsAsync(unsigned &aCounter, const EndpointVariant &aEndpointVariant, Utility::ConstBuffer aBuffer,
	RespondCb aRespondCb)
{
	for (auto &receiver : ReceiverImpl::ReceiverRegistry::getIterators()) {
		auto reducedEndpointVariant = aEndpointVariant;

		if (RoutingRules::getInstance().reduce(reducedEndpointVariant, receiver.endpointVariant)) {  // Check if this particular receiver should accept the request
			receiver.notify(aEndpointVariant, reducedEndpointVariant, aCounter, std::move(aRespondCb), aBuffer);
		}
	}
}

/// \brief Notify a receiver, and forward the message to the next receivers in an asynchronous manner.
/// `aBusyCounter` is a cross-process counter which should be increased with every pending notification sequence.
///
void Receiver::notify(const EndpointVariant &aSenderEndpointVariant, const EndpointVariant &aReducedEndpointVariant,
	unsigned &aBusyCounter, RespondCb aRespondCb, Utility::ConstBuffer aBuffer)
{
	Utility::ConstBuffer outBuffer = aBuffer;
	RespondCb outRespondCb{};

	// A stub wrapper which is used to replace the forwarded buffer, if a receiver pushes one of its own
	auto forwardCb =
		[&outBuffer, &outRespondCb](Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
		{
			outBuffer = aBuffer;
			outRespondCb = aRespondCb;
		};

	receiveCb(aSenderEndpointVariant, aBuffer, aRespondCb, forwardCb);  // Perform notification, set buffers to forward
	Utility::Comm::RaiiCounter raiiCounter{aBusyCounter};  // Increase the used busyCounter, since a new request is being queued
	Utility::Thr::Wq::MediumPriority::getInstance().push(
		[&aBusyCounter, aReducedEndpointVariant, raiiCounter, outBuffer, outRespondCb]() mutable
		{
			notifyAsAsync(aBusyCounter, aReducedEndpointVariant, outBuffer, std::move(outRespondCb));
		});
}

ReceiverImpl::Route::Route(const EndpointVariant &)
{
}

void ReceiverImpl::Route::lock()
{
	while (!tryLock()) {
		Utility::Tim::taskDelay(kNotifyWait);
	}
}

bool ReceiverImpl::Route::tryLock()
{
	std::lock_guard<std::mutex> lock{sRouteDetails.mutex};
	(void)lock;
	bool ret = (0 == sRouteDetails.counter);

	if (ret) {
		sRouteDetails.counter = 1;
	}

	return ret;
}

void ReceiverImpl::Route::unlock()
{
	sRouteDetails.counter = 0;
}

bool ReceiverImpl::Route::checkDone()
{
	return 1 == sRouteDetails.counter;
}

}  // namespace Bdg
