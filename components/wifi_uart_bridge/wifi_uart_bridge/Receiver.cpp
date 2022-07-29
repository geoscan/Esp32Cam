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

static constexpr std::size_t kReceiverStorageCapacity = 16;

static struct ReceiverRegistry {
	Utility::Comm::OrderedInstanceRegistry<Receiver> instances;
	std::mutex mutex;
} sReceiverRegistry{{16}, {}};

static struct RouteDetails {
	std::atomic_size_t turnBoundary;
	std::atomic_size_t turn;
} sRouteDetails {{0}, {}};

constexpr std::chrono::milliseconds kNotifyWait{20};

/// \brief Instantiate a notification chain and wait for it to get finished.
///
void Receiver::notifyAs(const EndpointVariant &aEndpointVariant, Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
{
	ReceiverImpl::Route route{aEndpointVariant};
	bool ongoing = false;

	Utility::Thr::Wq::MediumPriority::getInstance().pushContinuousWait(
		[&route, &ongoing, &aEndpointVariant, &aBuffer, &aRespondCb]()
		{
			bool ret = true;

			if (ongoing) {
				if (route.checkDone()) {
					route.unlock();
					ret = false;
				}
			} else if (route.tryLock()) {
				notifyAsImpl(route, aEndpointVariant, aBuffer, aRespondCb);
				ongoing = true;
			}

			return ret;
		});
}

/// \brief Same as `notifyAs`, but without waiting. When the caller's turn comes, `aGetBufferCb` will be called, and
/// it is expected to produce a message.
///
void Receiver::notifyAsAsync(const EndpointVariant &aEndpointVariant, GetBufferCb aGetBufferCb, RespondCb aRespondCb)
{
	ReceiverImpl::Route route{aEndpointVariant};
	bool ongoing = false;

	Utility::Thr::Wq::MediumPriority::getInstance().pushContinuous(
		[ongoing, route, aEndpointVariant, aGetBufferCb, aRespondCb]() mutable
		{
			bool ret = true;

			if (ongoing) {
				if (route.checkDone()) {
					route.unlock();
					ret = false;
				}
			} else if (route.tryLock()) {
				notifyAsImpl(route, aEndpointVariant, aGetBufferCb(), std::move(aRespondCb));
				ongoing = true;
			}

			return ret;
		});
}

Receiver::Receiver(const EndpointVariant &aIdentity, ReceiveCb &&aReceiveCb) :
	receiveCb{aReceiveCb},
	endpointVariant{aIdentity}
{
	std::lock_guard<std::mutex> lock{sReceiverRegistry.mutex};
	sReceiverRegistry.instances.add(*this);
}

Receiver::~Receiver()
{
	std::lock_guard<std::mutex> lock{sReceiverRegistry.mutex};
	sReceiverRegistry.instances.remove(*this);
}

/// \brief Determine which receivers are eligible for being notified upon an incoming message based on `RoutingRules`.
/// Notify each of them
///
void Receiver::notifyAsImpl(ReceiverImpl::Route aRoute, const EndpointVariant &aEndpointVariant,
	Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
{
	assert(RoutingRules::checkInstance());

	for (auto receiver : sReceiverRegistry.instances) {
		auto reduced = aEndpointVariant;

		if (RoutingRules::getInstance().reduce(reduced, receiver->endpointVariant)) {
			Utility::ConstBuffer outBuffer = aBuffer;
			RespondCb outRespondCb = aRespondCb;
			bool forwarded = false;
			auto forwardCb =
				[&forwarded, &outBuffer, &outRespondCb](Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
				{
					forwarded = true;
					outBuffer = aBuffer;
					outRespondCb = aRespondCb;
				};
			receiver->receiveCb(aEndpointVariant, aBuffer, aRespondCb, forwardCb);

			if (forwarded) {
				notifyAsImpl(aRoute, reduced, outBuffer, std::move(aRespondCb));
			}
		}
	}
}

ReceiverImpl::Route::Route(const EndpointVariant &) : turn{sRouteDetails.turnBoundary.fetch_add(1) + 1}
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
	bool ret = sRouteDetails.turn.load() == turn;

	if (ret) {
		sReceiverRegistry.mutex.lock();
	}

	return ret;
}

void ReceiverImpl::Route::unlock()
{
	auto turnExpected = turn;
	sRouteDetails.turn.compare_exchange_weak(turnExpected, turn + 1);

	if (turnExpected == turn) {
		sReceiverRegistry.mutex.unlock();
	}
}

bool ReceiverImpl::Route::checkDone()
{
	return true;
}

bool operator<(const Receiver &aLhs, const Receiver &aRhs)
{
	return aLhs.endpointVariant < aRhs.endpointVariant;
}

bool operator<(const Receiver &aLhs, const EndpointVariant &aRhs)
{
	return aLhs.endpointVariant < aRhs;
}

bool operator==(const Receiver &aLhs, const EndpointVariant &aRhs)
{
	return aLhs.endpointVariant == aRhs;
}

}  // namespace Bdg
