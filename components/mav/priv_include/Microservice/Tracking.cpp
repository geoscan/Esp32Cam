//
// Tracking.cpp
//
// Created on: Nov 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Tracking.hpp"
#include "Globals.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "Helper/MavlinkCommandAck.hpp"
#include "utility/time.hpp"

namespace Mav {
namespace Mic {

Tracking::Tracking() : key{{&Tracking::onMosseTrackerUpdate, this, false}}
{
}

Tracking::~Tracking()
{
	key.onMosseTrackerUpdate.setEnabled(false);
}

/// \brief Dispatches messages by message type. Decomposition method to make the code more comprehensible.
Microservice::Ret Tracking::process(mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	auto ret = Ret::Ignored;

	switch (aMessage.msgid) {
		case MAVLINK_MSG_ID_COMMAND_LONG:  // Falls through
		case MAVLINK_MSG_ID_COMMAND_INT: {
			mavlink_command_long_t mavlinkCommandLong = Hlpr::MavlinkCommandLong::makeFrom(aMessage);

			if (mavlinkCommandLong.target_component == Globals::getCompIdTracker()
					&& mavlinkCommandLong.target_system == Globals::getSysId()) {
				switch (mavlinkCommandLong.command) {
					case MAV_CMD_SET_MESSAGE_INTERVAL:
						ret = processSetMessageInterval(mavlinkCommandLong, aMessage, aOnResponse);

						break;

					default:
						break;
				}

			}
		}
	}

	return ret;
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

	if (static_cast<int>(aMavlinkCommandLong.param1) == MAVLINK_MSG_ID_CAMERA_TRACKING_IMAGE_STATUS
			&& aMavlinkCommandLong.target_component == Globals::getCompIdTracker()
			&& aMavlinkCommandLong.target_system == Globals::getSysId()) {
		ret = Microservice::Ret::Response;

		switch (static_cast<int>(aMavlinkCommandLong.param2)) {
			case kTrackingDisable:
				// Disable event response
				key.onMosseTrackerUpdate.setEnabled(false);

				break;

			case kTrackingEnablePerFrame:
				// Enable event response
				key.onMosseTrackerUpdate.setEnabled(true);

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

/// \brief Emits DEBUG_VECT messages containing info on tracking process
void Tracking::onMosseTrackerUpdate(Sub::Trk::MosseTrackerUpdate aMosseTrackerUpdate)
{
	mavlink_debug_vect_t mavlinkDebugVect = debugVectMakeFrom(aMosseTrackerUpdate);  // TODO: move to "mav helper" ??
	mavlink_message_t mavlinkMessage;
	const auto systemId = Globals::getSysId();
	const auto compId = Globals::getCompIdTracker();
	mavlink_msg_debug_vect_encode(systemId, compId, &mavlinkMessage, &mavlinkDebugVect);
	notify(mavlinkMessage);
}

/// \brief Constructs `mavlink_debug_vect_t` instance according to the description provided in (\see
/// `Microservice::Tracking`)
mavlink_debug_vect_t Tracking::debugVectMakeFrom(const Sub::Trk::MosseTrackerUpdate &aOnMosseTrackerUpdate)
{
	union FieldMap {
		struct {
			// Center
			std::uint16_t coord;
			// Roi size along the dimension
			std::uint16_t size;
		} decomposed __attribute__((packed));
		float f32;
		static_assert(sizeof(decomposed) == sizeof(float), "");
	};

	mavlink_debug_vect_t mavlinkDebugVect;
	mavlinkDebugVect.time_usec = Ut::bootTimeUs();
	{
		FieldMap fieldMap;
		fieldMap.decomposed.coord = aOnMosseTrackerUpdate.roiX + aOnMosseTrackerUpdate.roiWidth / 2;
		fieldMap.decomposed.size = aOnMosseTrackerUpdate.frameWidth;
		mavlinkDebugVect.x = fieldMap.f32;
	}
	{
		FieldMap fieldMap;
		fieldMap.decomposed.coord = aOnMosseTrackerUpdate.roiY + aOnMosseTrackerUpdate.roiHeight / 2;
		fieldMap.decomposed.size = aOnMosseTrackerUpdate.frameHeight;
		mavlinkDebugVect.y = fieldMap.f32;
	}
	mavlinkDebugVect.z = aOnMosseTrackerUpdate.psr;  // TODO: clamp PSR

	return mavlinkDebugVect;
}

}  // namespace Mic
}  // namespace Mav
