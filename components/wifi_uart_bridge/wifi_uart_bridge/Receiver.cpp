//
// Receiver.cpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)
#include <esp_log.h>

#include "Receiver.hpp"
#include "utility/cont/ListArray.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "utility/comm/RaiiCounter.hpp"
#include "utility/thr/WorkQueue.hpp"
#include "utility/comm/InstanceRegistry.hpp"
#include "utility/LogSection.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"

namespace Bdg {

constexpr std::chrono::milliseconds kNotifyWait{20};

/// \brief Instantiate a notification chain and wait for it to get finished.
///
void Receiver::notifyAs(NotifyCtx aCtx)
{
	GS_UTILITY_LOG_SECTIOND(Bdg::kDebugTag, "Receiver::notifyAs()");
	ReceiverImpl::Route route{aCtx.endpointVariant};
	bool ongoing = false;

	Utility::Thr::Wq::MediumPriority::getInstance().pushContinuousWait(
		[&route, &ongoing, &aCtx]()
		{
			bool ret = true;

			if (ongoing) {
				if (route.checkDone()) {
					Receiver::getReceiverRegistry().mutex.unlock();
					route.unlock();
					ret = false;
				}
			} else if (route.tryLock()) {
				Receiver::getReceiverRegistry().mutex.lock();
				notifyAsImpl(route, aCtx);
				ongoing = true;
			}

			return ret;
		});
}

/// \brief Same as `notifyAs`, but without waiting. When the caller's turn comes, `aGetBufferCb` will be called, and
/// it is expected to produce a message.
///
void Receiver::notifyAsAsync(AsyncNotifyCtx aCtx)
{
	GS_UTILITY_LOG_SECTIOND(Bdg::kDebugTag, "Receiver::notifyAsAsync()");
	ReceiverImpl::Route route{aCtx.endpointVariant};
	bool ongoing = false;

	Utility::Thr::Wq::MediumPriority::getInstance().pushContinuous(
		[ongoing, route, aCtx]() mutable
		{
			bool ret = true;

			if (ongoing) {
				if (route.checkDone()) {
					Receiver::getReceiverRegistry().mutex.unlock();
					route.unlock();
					ret = false;
				}
			} else if (route.tryLock()) {
				Receiver::getReceiverRegistry().mutex.lock();
				notifyAsImpl(route, {aCtx.endpointVariant, aCtx.getBufferCb(), std::move(aCtx.respondCb)});
				ongoing = true;
			}

			return ret;
		});
}

Receiver::Receiver(const EndpointVariant &aIdentity) :
	endpointVariant{aIdentity}
{
	std::lock_guard<std::mutex> lock{Receiver::getReceiverRegistry().mutex};
	Receiver::getReceiverRegistry().instances.add(*this);
}

Receiver::~Receiver()
{
	std::lock_guard<std::mutex> lock{Receiver::getReceiverRegistry().mutex};
	Receiver::getReceiverRegistry().instances.remove(*this);
}

/// \brief Determine which receivers are eligible for being notified upon an incoming message based on `RoutingRules`.
/// Notify each of them
///
void Receiver::notifyAsImpl(ReceiverImpl::Route aRoute, NotifyCtx aCtx)
{
	assert(RoutingRules::checkInstance());

	for (auto receiver : Receiver::getReceiverRegistry().instances) {
		auto reduced = aCtx.endpointVariant;

		if (RoutingRules::getInstance().reduce(reduced, receiver->endpointVariant)) {
			Utility::ConstBuffer outBuffer = aCtx.buffer;
			RespondCb outRespondCb = aCtx.respondCb;
			bool forwarded = false;
			auto forwardCb =
				[&forwarded, &outBuffer, &outRespondCb](Bdg::ForwardCtx aCtx)
				{
					forwarded = true;
					outBuffer = aCtx.buffer;
					outRespondCb = aCtx.respondCb;
				};
			auto bufferSlice = aCtx.buffer;

			// Notify the receiver, and, depending on whether it performs chunk-by-chunk or batch processing, re-notify it w/ a sliced buffer
			do {
				receiver->onReceive(OnReceiveCtx{aCtx.endpointVariant, bufferSlice, aCtx.respondCb,
					std::move(forwardCb)});

				if (forwarded) {
					notifyAsImpl(aRoute, {reduced, outBuffer, std::move(aCtx.respondCb)});
				}
			} while (bufferSlice.size() > 0 && bufferSlice.data() != aCtx.buffer.data());  // If `Receiver` has sliced the buffer, notify until it is fully consumed
		}
	}
}

Receiver::ReceiverRegistry &Receiver::getReceiverRegistry()
{
	static constexpr std::size_t kReceiverStorageCapacity = 16;
	static ReceiverRegistry receiverRegistry{{kReceiverStorageCapacity}, {}};

	return receiverRegistry;
}

/// \brief API for data processing. A receiver is expected to show a certain behavior:
/// 1) `ForwardCb` callback should only be forwarded when `Receiver` instance does not consume the buffer.
/// 2) `Buffer` reference is a mutable buffer. The instance may (A) leave it unscathed, or (B) `slice` the buffer. If
///    (B) option is chosen, the instance will be revisited until it consumes the entire buffer.
///
/// \details Effectively, 1) provides a leverage over a notification process enabling the instance to define whether
/// the chain will be continued or terminated, although this way of using the notification chain is discouraged,
/// because it is to difficult to reach the agreement between the participants of a notification process, when this
/// way of forwarding the messages is used.
///
/// 2) is used, because there are cases (see MAVLink) when `Receiver` has to process input buffer chunk-by-chunk. Take
/// `mav` module as an example: there may be 2 messages in a single buffer, and without this revisiting technique the
/// parser is rendered unable to process both of them.
///
void Receiver::onReceive(OnReceiveCtx)
{
}

ReceiverImpl::Route::Route(const EndpointVariant &) : turn{Route::getRouteDetails().turnBoundary.fetch_add(1)}
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
	bool ret = Route::getRouteDetails().turn.load() == turn;

	if (ret) {
		Receiver::getReceiverRegistry().mutex.lock();
	}

	return ret;
}

void ReceiverImpl::Route::unlock()
{
	auto turnExpected = turn;
	Route::getRouteDetails().turn.compare_exchange_weak(turnExpected, turn + 1);

	if (turnExpected == turn) {
		Receiver::getReceiverRegistry().mutex.unlock();
	}
}

bool ReceiverImpl::Route::checkDone()
{
	return true;
}

ReceiverImpl::Route::RouteDetails &ReceiverImpl::Route::getRouteDetails()
{
	static RouteDetails routeDetails {{0}, {0}};

	return routeDetails;
}

bool operator<(const Receiver &aLhs, const Receiver &aRhs)
{
	return aLhs.endpointVariant.operator<(aRhs.endpointVariant);
}

bool operator<(const Receiver &aLhs, const EndpointVariant &aRhs)
{
	return aLhs.endpointVariant.operator<(aRhs);
}

bool operator==(const Receiver &aLhs, const EndpointVariant &aRhs)
{
	return aLhs.endpointVariant.operator==(aRhs);
}

LambdaReceiver::LambdaReceiver(const EndpointVariant &aEndpointVariant, ReceiveCb &&aReceiveCb) :
	Receiver{aEndpointVariant}, receiveCb{aReceiveCb}
{
}

void LambdaReceiver::onReceive(OnReceiveCtx aCtx)
{
	receiveCb(std::forward<OnReceiveCtx>(aCtx));
}

}  // namespace Bdg
