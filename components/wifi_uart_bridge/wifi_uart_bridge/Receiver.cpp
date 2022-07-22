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

constexpr std::chrono::milliseconds kNotifyWait{20};

template <class T>
using ContainerType = typename Utility::Cont::ListArray<T, 16>;  ///< List array will expand in a memory-safe manner, w/o devalidating already stored instances
static Utility::Comm::SyncedStorage<unsigned, ContainerType> sCounterRegistry{};  ///< Storage of busy counters that is guaranteed to outlive its users

/// \brief Instantiate a notification chain and wait for it to get finished.
///
void Receiver::notifyAs(const EndpointVariant &aEndpointVariant, Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
{
	unsigned counter = 0;
	notifyAsAsync(counter, aEndpointVariant, aBuffer, std::move(aRespondCb));

	while (counter > 0) {
	}
}

Receiver::Receiver(const EndpointVariant &aIdentity, ReceiveCb &&aReceiveCb) :
	receiveCb{aReceiveCb},
	endpointVariant{aIdentity},
	busyCounter{sCounterRegistry.emplace(0u)}
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
			if (receiver.busyCounter > 0) {
				receiver.notifyDelayed(aEndpointVariant, reducedEndpointVariant, aCounter, std::move(aRespondCb),
					aBuffer);
			} else {
				receiver.notify(aEndpointVariant, reducedEndpointVariant, aCounter, std::move(aRespondCb), aBuffer);
			}
		}
	}
}

/// \brief Builds an expected route based on a starting point.
///
/// \details MEMORY OVERHEAD. The routing solves the problem of maintaining buffers' consistency across receivers.
/// There is an alternative that is to just copy the content that is being passed b/w endpoints, and maintain those
/// buffers instead of implementing synchronization mechanism. However, this approach is not acceptible due to the time
/// and memory overhead it entails.
///
/// Other options are: (1) synchronize buffers, or (2) accept drops and content corruption. It's been decided to use
/// the first one
///
/// SYNCHRONIZATION. Therefore, the remaining solution is to ensure buffer consistency through synchronization. It
/// leaves the following options to that would solve this problem:
///
/// 1. Common lock: lock the entire synchronization process, so only one synchronization sequence may be active at a
/// moment;
///
/// 2. Group lock (generalization over common lock): divide endpoints into groups, determine which group will
/// participate in the process, and lock it;
///
/// 3. Route lock (more tailored group lock - only lock those that we want to use): determine which endpoints will
/// participate in routing based on first endpoint, lock them in a pre-determined sequence (thus avoiding
/// deadlocks), and execute the notification process.
///
/// The benefits of each of those are unclear at the moment. However, mocking the third one encompasses all of them. So
/// it has been decided to use the first one, considering that it proved itself viable in practice, while leaving the
/// room for improvement by adopting an extendable architectural approach allowing implementing 2nd or 3rd option
/// later.
///
Receiver::ExpectedRoute Receiver::buildRoute(const EndpointVariant &)
{
	return nullptr;
}

void Receiver::lockRoute(const ExpectedRoute &)
{
}

bool Receiver::tryLockRoute(const ExpectedRoute &)
{
	return false;
}

/// \brief A Receiver's `busyCounter = 0`, so it should be notified right away.
///
void Receiver::notify(const EndpointVariant &aSenderEndpointVariant, const EndpointVariant &aReducedEndpointVariant,
	unsigned &aBusyCounter, RespondCb aRespondCb, Utility::ConstBuffer aBuffer)
{
	Utility::ConstBuffer outBuffer = aBuffer;
	RespondCb outRespondCb{};

	// A stub wrapper which is used to determine whether the receiver initiates a new notification chain w/ its own
	// buffer, or it just has ignored or forwarded the same buffer. The former means that the originator's buffer can be
	// released (and the waiting loop should be detached).
	//
	auto forwardCb =
		[&outBuffer, &outRespondCb](Utility::ConstBuffer aBuffer, RespondCb aRespondCb)
		{
			outBuffer = aBuffer;
			outRespondCb = aRespondCb;
		};

	receiveCb(aSenderEndpointVariant, aBuffer, aRespondCb, forwardCb);  // Perform notification
	const bool detached = outBuffer.data() != aBuffer.data();  // The receiver has forwarded its own buffer (e.g. consumed and wrapped previously received one).
	unsigned &busyCounterSwitch = detached ? busyCounter : aBusyCounter;   // If some other buffer is used, the previous notification chain gets terminated by gradually decreasing the busy counter notifier-by-notifier
	Utility::Comm::RaiiCounter raiiCounter{busyCounterSwitch};  // Increase the used busyCounter, since a new request is being queued
	Utility::Thr::Wq::MediumPriority::getInstance().push(
		[&busyCounterSwitch, aReducedEndpointVariant, raiiCounter, outBuffer, outRespondCb]() mutable
		{
			notifyAsAsync(busyCounterSwitch, aReducedEndpointVariant, outBuffer, std::move(outRespondCb));
		});
}

/// \brief A Receiver's `busyCounter > 0`, therefore, it should be waited to become equal zero, or for the Receiver to
/// get destructed. The request is queued in a working queue.
///
void Receiver::notifyDelayed(const EndpointVariant &aSender, const EndpointVariant &aReducedEndpointVariant,
	unsigned &aBusyCounter, RespondCb aRespondCb, Utility::ConstBuffer aBuffer)
{
	auto lockWrap = asLockWrap();
	Utility::Comm::RaiiCounter raiiCounter{aBusyCounter};

	Utility::Thr::Wq::MediumPriority::getInstance().pushContinuous(
		[&aBusyCounter, lockWrap, raiiCounter, aSender, aReducedEndpointVariant, aRespondCb, aBuffer]() mutable
		{
			// Lock the receiver
			auto wrap = lockWrap.wrap();
			auto wrapped = wrap.tryGet();
			bool ret = true;

			if (nullptr == wrapped) {  // The object has been removed from the queue (destructed). Terminate
				ret = false;
			} else if (0 == wrapped->busyCounter) {  // All the pending notification have finished, get on to notification
				wrapped->notify(aSender, aReducedEndpointVariant, aBusyCounter, aRespondCb, aBuffer);
				ret = false;
			}

			return ret;
		});
}

}  // namespace Bdg
