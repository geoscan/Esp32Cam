//
// Receiver.hpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_RECEIVER_HPP_)
#define WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_RECEIVER_HPP_

#include "wifi_uart_bridge/EndpointVariant.hpp"
#include "utility/Buffer.hpp"
#include <Rr/Util/Module.hpp>
#include <list>
#include <functional>

namespace Bdg {

class Receiver;

namespace ReceiverImpl {

/// \brief Defines access policy
///
struct SyncTrait {
	using Mutex = std::mutex;
	static constexpr auto kStoragePolicy = Rr::Sync::Policy::Type::Mutex;  ///< Static storage of `Receiver` instances is synchronized w/ a mutex
	static constexpr auto kSharedAccessPolicy = Rr::Sync::Policy::Type::Mutex;  ///< Access to a particular instance of `Receiver` is synchronized w/ a mutex
};

using ReceiverRegistry = Rr::Util::ModuleRegistry<Receiver, SyncTrait, std::list>;

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
/// participate in routing based on starting endpoint, lock them in a pre-determined sequence (thus avoiding
/// deadlocks), and execute the notification process.
///
/// The benefits of each of those are unclear at the moment. However, mocking the third one encompasses all of them. So
/// it has been decided to use the first one, considering that it proved itself viable in practice, while leaving the
/// room for improvement by adopting an extendable architectural approach allowing implementing 2nd or 3rd option
/// later.
///
struct Route {
	Route(const EndpointVariant &);
	void lock();
	bool tryLock();
	void unlock();
	bool checkDone();
};

}  // namespace ReceiverImpl

using RespondCb = std::function<void(Utility::ConstBuffer)>;
using ForwardCb = std::function<void(Utility::ConstBuffer, RespondCb)>;
using ReceiveCb = std::function<void(const EndpointVariant & /*sender*/, Utility::ConstBuffer, RespondCb, ForwardCb)>;

/// \brief A receiver is an "observer" in Observer pattern. On creation, it gets registered in a queue under an
/// identity, and then notified according to a list of "request-reponse-reduce" rules defined by `Bdg::RoutingRules`.
///
/// \details The work of an observer is to receive a sequence (if one is addressed to it), and either ignore, modify,
/// or forward it further. An observer is only aware of its identity and the identitity of a sender from which it
/// receives a message.
///
/// ONE NOTIFICATION AT A TIME. At a time, only one notification sequence is active: `notifyAs` in an entry point, and
/// it is synchronized w/ a static mutex.
///
/// RECEIVER HOLDS ITS OWN BUFFER. If a Receiver initiates a new notification sequence (processes a received buffer and
/// composes a new message), it is expected to use its own buffer.
///
/// SYNC. This entire scheme with (1) busy counter and (2) use of a shared work queue (a) protects Receiver's buffers
/// from being modified while in use, and (b) prevent deadlocks upon Receiver instances.
///
class Receiver : public Rr::Util::MakeModule<typename ReceiverImpl::ReceiverRegistry> {
public:
	static void notifyAs(const EndpointVariant &, Utility::ConstBuffer, RespondCb);
	Receiver(const EndpointVariant &aIdentity, ReceiveCb &&aReceiveCb);

private:
	static void notifyAsAsync(unsigned &counter, const EndpointVariant &, Utility::ConstBuffer, RespondCb);
	void notify(const EndpointVariant &aSender, const EndpointVariant &aReducedEndpointVariant, unsigned &busyCounter,
		RespondCb aRespondCb, Utility::ConstBuffer aBuffer);

private:
	ReceiveCb receiveCb;
	EndpointVariant endpointVariant;
};

}  // namespace Bdg

#endif // WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_RECEIVER_HPP_
