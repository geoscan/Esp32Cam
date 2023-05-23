//
// DelayedMavlinkMessageInitialization.hpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_MAV_PRIV_INCLUDE_DELAYEDMAVLINKMESSAGEINITIALIZATION_HPP
#define COMPONENTS_MAV_PRIV_INCLUDE_DELAYEDMAVLINKMESSAGEINITIALIZATION_HPP

#include "Mavlink.hpp"

namespace Mav {

/// \brief From the perspective of memory expenses, it is cheaper to move
/// message initialization into a separate module.
struct DelayedMavlinkMessageInitialization {
	virtual void initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage) = 0;
};

}  // namespace Mav

#endif // COMPONENTS_MAV_PRIV_INCLUDE_DELAYEDMAVLINKMESSAGEINITIALIZATION_HPP
