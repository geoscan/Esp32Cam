//
// Receiver.cpp
//
// Created on: Jul 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL (CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)
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

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::Receiver, notifyAs, 0)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::Receiver, notifyAsAsync, 0)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::Receiver, notifyAsImpl, 0)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::ReceiverImpl::Route, tryLock, 0)
GS_UTILITY_LOGV_METHOD_SET_ENABLED(Bdg::ReceiverImpl::Route, unlock, 0)

enum class LogAspect {
	RoutingLock,
	Lifecycle,
};

GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Bdg::ReceiverImpl::Route, LogAspect::RoutingLock, 1)
GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Bdg::ReceiverImpl::Route, LogAspect::Lifecycle, 1)
GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Bdg::Receiver, LogAspect::RoutingLock, 1)

namespace Bdg {

constexpr std::chrono::milliseconds kNotifyWait{20};

/// \brief Instantiate a notification chain and wait for it to get finished.
///
void Receiver::notifyAs(NotifyCtx aCtx)
{
#if CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL >= 5
	aCtx.endpointVariant.logv("Receiver::notifyAs: ");
#endif

	ReceiverImpl::Route route{aCtx.endpointVariant};
	bool ongoing = false;

	Ut::Thr::Wq::MediumPriority::getInstance().pushContinuousWait(
		[&route, &ongoing, &aCtx]()
		{
			GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAs, "WorkQueue task is ongoing");
			bool ret = true;

			if (ongoing) {
				if (route.checkDone()) {
					route.unlock();
					GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAs, "Unlocked");
					ret = false;
				} else {
					GS_UTILITY_LOGV_CLASS_ASPECT(Bdg::kDebugTag, Receiver, LogAspect::RoutingLock,
						"Waiting for the route to get completed");
				}
			} else if (route.tryLock()) {
				GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAs, "Acquired the lock");
				notifyAsImpl(route, aCtx);
				ongoing = true;
			} else {
				GS_UTILITY_LOG_EVERY_N_TURNS(5, GS_UTILITY_LOGV_CLASS_ASPECT(Bdg::kDebugTag,
					Receiver, LogAspect::RoutingLock, "Waiting for lock"); )
			}

			return ret;
		});
}
/// \brief Same as `notifyAs`, but without waiting. When the caller's turn comes, `aGetBufferCb` will be called, and
/// it is expected to produce a message.
///
void Receiver::notifyAsAsync(AsyncNotifyCtx aCtx)
{
#if CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL >= 5
	aCtx.endpointVariant.logv("Receiver::notifyAsAsync: ");
#endif

	ReceiverImpl::Route route{aCtx.endpointVariant};
	bool ongoing = false;

	Ut::Thr::Wq::MediumPriority::getInstance().pushContinuous(
		[ongoing, route, aCtx]() mutable
		{
			bool ret = true;
			GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsAsync, "WorkQueue task is ongoing");

			if (ongoing) {
				if (route.checkDone()) {
					GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsAsync, "Releasing the lock");
					route.unlock();
					ret = false;
				}
			} else if (route.tryLock()) {
				GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsAsync, "Acquired the lock");
				notifyAsImpl(route, {aCtx.endpointVariant, aCtx.getBufferCb(), std::move(aCtx.respondCb)});
				ongoing = true;
			}

			return ret;
		});
}

Receiver::Receiver(const EndpointVariant &aIdentity) : Receiver{aIdentity, ""}
{
}

Receiver::Receiver(const EndpointVariant &aIdentity, const char *aName) : endpointVariant{aIdentity}, name{aName}
{
	Ut::Thr::Wq::MediumPriority::getInstance().push(
		[this]()
		{
			Receiver::getReceiverRegistry().instances.add(*this);
		});
}

Receiver::~Receiver()
{
	Ut::Thr::Wq::MediumPriority::getInstance().pushWait(
		[this]()
		{
			Receiver::getReceiverRegistry().instances.remove(*this);
		});
}

/// \brief Determine which receivers are eligible for being notified upon an incoming message based on `RoutingRules`.
/// Notify each of them
///
void Receiver::notifyAsImpl(ReceiverImpl::Route aRoute, NotifyCtx aCtx)
{
	GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsImpl, "starting notification cycle");
	for (auto receiver : Receiver::getReceiverRegistry().instances) {
		auto reduced = aCtx.endpointVariant;

		if (RoutingRules::getInstance().reduce(reduced, receiver->endpointVariant)) {
			GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsImpl,
				"found a suitable reduction for receiver \"%s\"", receiver->getName());

#if CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL >= 5
			aCtx.endpointVariant.logv("Receiver::notifyAsImpl() Reduction source: ");
			receiver->endpointVariant.logv("Receiver::notifyAsImpl() Reduction destination: ");
			reduced.logv("Receiver::notifyAsImpl() Reduced as: ");
#endif

			Ut::Cont::ConstBuffer outBuffer = aCtx.buffer;
			RespondCb outRespondCb = aCtx.respondCb;
			bool forwarded = false;
			auto forwardCb =
				[&forwarded, &outBuffer, &outRespondCb](Bdg::ForwardCtx aCtx)
				{
					forwarded = true;
					outBuffer = aCtx.buffer;  // A receiver may override buffer that is being used for notification
					outRespondCb = aCtx.respondCb;  // A receiver may override response function
				};
			auto bufferSlice = aCtx.buffer;

			// Notify the receiver, and, depending on whether it performs chunk-by-chunk or batch processing, re-notify it w/ a sliced buffer
			do {
				GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsImpl, "feeding the buffer %d bytes remaining",
					aCtx.buffer.size());
				receiver->onReceive(OnReceiveCtx{aCtx.endpointVariant, bufferSlice, aCtx.respondCb, forwardCb});

				if (forwarded) {
					GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsImpl,
						"forwarded, initializing new notification cycle...");
					notifyAsImpl(aRoute, {reduced, outBuffer, outRespondCb});
				}
			} while (bufferSlice.size() > 0 && bufferSlice.data() != aCtx.buffer.data());  // A receiver either slices buffer, replaces it, or leaves unscathed. In the former case, it will be notified until the buffer is sliced to `size()==0`
			GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsImpl, "finished forwarding");
		} else {
			GS_UTILITY_LOGV_METHOD(Bdg::kDebugTag, Receiver, notifyAsImpl,
				"could not find reduction rule for receiver \"%s\"", receiver->getName());
		}
	}
}

Receiver::ReceiverRegistry &Receiver::getReceiverRegistry()
{
	static constexpr std::size_t kReceiverStorageCapacity = 16;
	static ReceiverRegistry receiverRegistry{{kReceiverStorageCapacity}};

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

ReceiverImpl::Route::Route(const EndpointVariant &)
{
}

ReceiverImpl::Route::~Route()
{
}

void ReceiverImpl::Route::lock()
{
}

bool ReceiverImpl::Route::tryLock()
{
	return true;
}

void ReceiverImpl::Route::unlock()
{
}

bool ReceiverImpl::Route::checkDone()
{
	return true;
}

ReceiverImpl::Route::RouteDetails &ReceiverImpl::Route::getRouteDetails()
{
	static RouteDetails routeDetails {};

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

LambdaReceiver::LambdaReceiver(const EndpointVariant &aEndpointVariant, const char *aName, ReceiveCb &&aReceiveCb) :
	Receiver{aEndpointVariant, aName},
	receiveCb{aReceiveCb}
{
}

void LambdaReceiver::onReceive(OnReceiveCtx aCtx)
{
	receiveCb(std::forward<OnReceiveCtx>(aCtx));
}

}  // namespace Bdg
