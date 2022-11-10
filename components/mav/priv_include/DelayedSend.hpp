//
// DelayedSend.hpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_DELAYEDSEND_HPP_
#define MAV_PRIV_INCLUDE_DELAYEDSEND_HPP_

#include "Mavlink.hpp"
#include "utility/Subscription.hpp"
#include "mapbox/variant.hpp"

namespace Mav {

using DelayedSendAsyncVariant = typename mapbox::util::variant<mavlink_camera_tracking_image_status_t>;

/// \brief Enables memory-efficient asynchronous messaging
///
/// \details Notification of MAVLink receivers (i.e. interfaces) is achieved throught a work queue w/ a subsequent
/// waitiing for the notification sequence to finish. Thus, calling notify in the context of a work queue task produces
/// dead lock, because the work queue becomes unable to proceeed due to active waiting.
///
/// The alternative is to notify in an asynchronous manner, without waiting, but it'd require copying of data. To spare
/// ourselves the memory expenses, the payload is thus stored as a compact mavlink structure.
struct DelayedSendAsyncCtx {
	/// System id (the one that is used in MAVLink messages, http://mavlink.io/en/guide/serialization.html)
	std::uint8_t sysId;
	/// Component id
	std::uint8_t compId;
	/// MAVGen-generated struct
	DelayedSendAsyncVariant delayedSendAsyncVariant;

	template <class ...Ts>
	DelayedSendAsyncCtx(std::uint8_t aSysId, std::uint8_t aCompId, Ts &&...aArgs) : sysId{aSysId}, compId{aCompId},
		delayedSendAsyncVariant{std::forward<Ts>(aArgs)...}
	{
	}
};

/// \brief The receiver in a primitive subscriber / receiver scheme.
///
/// \details Delayed send enables sending messages outside the "request / response" communication scheme. It allows
/// sending periodic or sporadic messages, while leaving the handling of those to the receiver (in that case - an
/// instance of `Mav::Dispatcher`.
class DelayedSendHandle {
public:
	virtual void onSubscription(const mavlink_message_t &) = 0;
	/// \brief Same as `onSubscription(const mavlink_message_t &)`, but prepares the messages to be send at the time
	/// when the messaging "pipeline" becomes available.
	///
	/// \details The current communication scheme uses a work queue to dispatch messages between recepients, and MAVLink
	/// is one of the many participants. It is sometimes required to send a message FROM the context of a work queue,
	/// which would cause a deadlock. \sa `Mav::DelayedSendAsyncCtx` for more info.
	virtual void onSubscription(DelayedSendAsyncCtx delayedSendAsyncCtx) = 0;
	virtual ~DelayedSendHandle() = default;
};

/// \brief Enables one to send a MAVLink message outside the request/response scheme. Useful for implementing telemetry
/// messages sending
using DelayedSend = Ut::Sub::Sender1to1<DelayedSendHandle>;

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_DELAYEDSEND_HPP_
