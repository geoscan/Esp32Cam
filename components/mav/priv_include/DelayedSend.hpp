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

namespace Mav {

class DelayedSendHandle {
public:
	virtual void onSubscription(const mavlink_message_t &) = 0;
	virtual ~DelayedSendHandle() = default;
};

///
/// \brief Enables one to send a MAVLink message outside the request/response scheme. Useful for implementing telemetry
/// messages sending
///
using DelayedSend = Utility::Sub::Sender1to1<DelayedSendHandle, const mavlink_message_t &>;

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_DELAYEDSEND_HPP_
