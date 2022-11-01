//
// Tracking.cpp
//
// Created on: Nov 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Tracking.hpp"
#include "Globals.hpp"
#include "Helper/MavlinkCommandAck.hpp"

namespace Mav {
namespace Mic {

Tracking::Tracking() : key{{&Tracking::onMosseTrackerUpdate, this, false}}
{
}

/// \brief The MAVLink tracking info message ("debug vector") only gets sent, when a tracker emits an event stating
/// that a frame has been processed successfully. `processSetMessageInterval` enables / disables response to that
/// event, and sends ACK according to the "command protocol".
Microservice::Ret Tracking::processSetMessageInterval(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	auto ret = Microservice::Ret::Ignored;
	constexpr int kTrackingDisable = -1;  // Disabled (https://mavlink.io/en/messages/common.html#MAV_CMD_SET_MESSAGE_INTERVAL)
	constexpr int kTrackingEnablePerFrame = 0;  // Default frequency (https://mavlink.io/en/messages/common.html#MAV_CMD_SET_MESSAGE_INTERVAL)
	auto result = MAV_RESULT_ACCEPTED;

	if (static_cast<int>(aMavlinkCommandLong.param1) == MAVLINK_MSG_ID_DEBUG_VECT
			&& aMavlinkCommandLong.target_component == Globals::getCompIdTracker()
			&& aMavlinkCommandLong.target_system == Globals::getSysId()) {
		switch (static_cast<int>(aMavlinkCommandLong.param2)) {
			case kTrackingDisable:
				// Disable event response
				key.onMosseTrackerUpdate.setEnabled(false);
				ret = Microservice::Ret::Response;

				break;

			case kTrackingEnablePerFrame:
				// Enable event response
				key.onMosseTrackerUpdate.setEnabled(true);
				ret = Microservice::Ret::Response;

				break;

			default:
				result = MAV_RESULT_DENIED;
		}
	}

	{
		auto ack = Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, result);
		ack.packInto(aMessage, Globals::getCompIdTracker());
		aOnResponse(aMessage);
	}

	return ret;
}

}  // namespace Mic
}  // namespace Mav
