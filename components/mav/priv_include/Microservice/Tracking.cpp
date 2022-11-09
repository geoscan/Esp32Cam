//
// Tracking.cpp
//
// Created on: Nov 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t

// TODO: tracking does not always start right away

#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "Tracking.hpp"
#include "Globals.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "Helper/MavlinkCommandAck.hpp"
#include "utility/time.hpp"
#include "utility/al/Algorithm.hpp"
#include "utility/LogSection.hpp"
#include "utility/thr/WorkQueue.hpp"
#include "module/ModuleBase.hpp"
#include "mav/mav.hpp"

GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Mav::Mic::Tracking, "messaging", 1);
GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Mav::Mic::Tracking, "messaging", 1);
GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(Mav::Mic::Tracking, "other", 1);
GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Mav::Mic::Tracking, "other", 1);

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
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Tracking, "messaging", "Command long "
				"target system %d  target component %d  command id %d", mavlinkCommandLong.target_system,
				mavlinkCommandLong.target_component, mavlinkCommandLong.command);

			if (mavlinkCommandLong.target_component == Globals::getCompIdTracker()
					&& mavlinkCommandLong.target_system == Globals::getSysId()) {
				switch (mavlinkCommandLong.command) {
					case MAV_CMD_SET_MESSAGE_INTERVAL:
						ret = processCmdSetMessageInterval(mavlinkCommandLong, aMessage, aOnResponse);

						break;

					case  MAV_CMD_CAMERA_TRACK_RECTANGLE:
						ret = processCmdCameraTrackRectangle(mavlinkCommandLong, aMessage, aOnResponse);

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
Microservice::Ret Tracking::processCmdSetMessageInterval(mavlink_command_long_t &aMavlinkCommandLong,
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

Microservice::Ret Tracking::processCmdCameraTrackRectangle(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	ESP_LOGI(Mav::kDebugTag, "Tracking: track rectangle request  left %.3ff  top %.3f  right %.3f  bottom %.3f",
		aMavlinkCommandLong.param1, aMavlinkCommandLong.param2, aMavlinkCommandLong.param3, aMavlinkCommandLong.param4);
	auto result = MAV_RESULT_ACCEPTED;

	if (!(aMavlinkCommandLong.param1 < aMavlinkCommandLong.param3
			&& aMavlinkCommandLong.param2 < aMavlinkCommandLong.param4
			&& aMavlinkCommandLong.param3 <= 1.0f
			&& aMavlinkCommandLong.param4 <= 1.0f
			&& aMavlinkCommandLong.param1 >= 0.0f
			&& aMavlinkCommandLong.param2 >= 0.0f
			&& aMavlinkCommandLong.param3 >= 0.0f
			&& aMavlinkCommandLong.param4 >= 0.0f)) {
		ESP_LOGW(Mav::kDebugTag, "Tracking: failed geometry check");
		result = MAV_RESULT_DENIED;
	} else {
		cameraState.fetch();
		// TODO: process fetch failure
		// Convert normalized values to the absolute ones
		const auto topLeftX = static_cast<std::uint16_t>(Ut::Al::scale(aMavlinkCommandLong.param1, 0,
			cameraState.frameWidth));
		const auto topLeftY = static_cast<std::uint16_t>(Ut::Al::scale(aMavlinkCommandLong.param2, 0,
			cameraState.frameHeight));
		const auto bottomRightX = static_cast<std::uint16_t>(Ut::Al::scale(aMavlinkCommandLong.param3, 0,
			cameraState.frameWidth));
		const auto bottomRightY = static_cast<std::uint16_t>(Ut::Al::scale(aMavlinkCommandLong.param4, 0,
			cameraState.frameHeight));
		// Prepare and set module field
		std::array<std::uint16_t, 4> rect = {{topLeftX, topLeftY,
			static_cast<std::uint16_t>(bottomRightX - topLeftX),
			static_cast<std::uint16_t>(bottomRightY - topLeftY)}};
		GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Tracking, "messaging",
			"Tracking: left %d  top %d  right %d  bottom %d  frame height %d  frame width %d", topLeftX, topLeftY,
			bottomRightX, bottomRightY, cameraState.frameHeight, cameraState.frameWidth);
		const auto nnotified = Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Tracking, Mod::Fld::Field::Roi>(rect,
			[&result](Mod::Fld::WriteResp aResp) mutable
			{
				if (!aResp.isOk()) {
					result = MAV_RESULT_FAILED;
				}
			});

		if (nnotified == 0) {
			ESP_LOGW(Mav::kDebugTag, "Unable to access tracking module");
			result = MAV_RESULT_FAILED;
		}
	}

	{
		auto ack = Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, result);
		ack.packInto(aMessage, Globals::getCompIdTracker());
		aOnResponse(aMessage);
	}

	if (result == MAV_RESULT_ACCEPTED && Ut::Thr::Wq::MediumPriority::checkInstance()) {
		Ut::Thr::Wq::MediumPriority::getInstance().push(
			[this]()
			{
				key.onMosseTrackerUpdate.setEnabled(true);  // TODO it causes deadlock
			});
	}

	return Ret::Response;
}

/// \brief Stops tracking, if any takes place
Microservice::Ret Tracking::processCmdCameraStopTracking(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	auto result = MAV_RESULT_FAILED;
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Tracking, Mod::Fld::Field::Initialized>(false,
		[&result](Mod::Fld::WriteResp aResp) mutable
		{
			if (aResp.isOk()) {
				result = MAV_RESULT_ACCEPTED;
			}
		});
	{
		auto ack = Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, result);
		ack.packInto(aMessage, Globals::getCompIdTracker());
		aOnResponse(aMessage);
	}

	return Ret::Response;
}

/// \brief Emits CAMERA_TRACKING_IMAGE_STATUS messages containing info on tracking process
void Tracking::onMosseTrackerUpdate(Sub::Trk::MosseTrackerUpdate aMosseTrackerUpdate)
{
	GS_UTILITY_LOGV_CLASS_ASPECT(Mav::kDebugTag, Tracking, "other", "onMosseTrackerUpdate");
	mavlink_camera_tracking_image_status_t mavlinkCameraTrackingImageStatus{};
	// Init the message
	mavlinkCameraTrackingImageStatus.rec_bottom_x = Ut::Al::normalize(aMosseTrackerUpdate.roiX, 0,
		aMosseTrackerUpdate.frameWidth);
	mavlinkCameraTrackingImageStatus.rec_bottom_y = Ut::Al::normalize(aMosseTrackerUpdate.roiY, 0,
		aMosseTrackerUpdate.frameHeight);
	mavlinkCameraTrackingImageStatus.rec_top_x = Ut::Al::normalize(aMosseTrackerUpdate.roiX
		+ aMosseTrackerUpdate.roiWidth, 0, aMosseTrackerUpdate.frameWidth);
	mavlinkCameraTrackingImageStatus.rec_top_y = Ut::Al::normalize(aMosseTrackerUpdate.roiY
		+ aMosseTrackerUpdate.roiHeight, 0, aMosseTrackerUpdate.frameHeight);
	mavlinkCameraTrackingImageStatus.rec_bottom_x = Ut::Al::clamp(mavlinkCameraTrackingImageStatus.rec_bottom_x, 0.0f,
		1.0f);
	mavlinkCameraTrackingImageStatus.rec_bottom_y = Ut::Al::clamp(mavlinkCameraTrackingImageStatus.rec_bottom_y, 0.0f,
		1.0f);
	mavlinkCameraTrackingImageStatus.rec_top_x = Ut::Al::clamp(mavlinkCameraTrackingImageStatus.rec_top_x, 0.0f, 1.0f);
	mavlinkCameraTrackingImageStatus.rec_bottom_x = Ut::Al::clamp(mavlinkCameraTrackingImageStatus.rec_bottom_x, 0.0f,
		1.0f);
	mavlinkCameraTrackingImageStatus.rec_bottom_y = Ut::Al::clamp(mavlinkCameraTrackingImageStatus.rec_bottom_y, 0.0f,
		1.0f);
	mavlinkCameraTrackingImageStatus.rec_top_y = Ut::Al::clamp(mavlinkCameraTrackingImageStatus.rec_top_y, 0.0f, 1.0f);
	// TODO PSR to infer tracking status
	mavlinkCameraTrackingImageStatus.tracking_status = CAMERA_TRACKING_STATUS_FLAGS_ACTIVE;
	mavlinkCameraTrackingImageStatus.tracking_mode = CAMERA_TRACKING_MODE_RECTANGLE;
	mavlinkCameraTrackingImageStatus.target_data = CAMERA_TRACKING_TARGET_DATA_IN_STATUS;
	mavlinkCameraTrackingImageStatus.point_x = std::numeric_limits<float>::quiet_NaN();
	mavlinkCameraTrackingImageStatus.point_y = std::numeric_limits<float>::quiet_NaN();
	mavlinkCameraTrackingImageStatus.radius = std::numeric_limits<float>::quiet_NaN();
	// Pack the message
	mavlink_message_t mavlinkMessage;
	const auto systemId = Globals::getSysId();
	const auto compId = Globals::getCompIdTracker();
	mavlink_msg_camera_tracking_image_status_encode(systemId, compId, &mavlinkMessage,
		&mavlinkCameraTrackingImageStatus);
	// Send
	notify(mavlinkMessage);
}

void Tracking::CameraState::fetch()
{
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[this](std::pair<int, int> aFrameSize)
		{
			frameWidth = std::get<0>(aFrameSize);
			frameHeight = std::get<1>(aFrameSize);
		});
}

}  // namespace Mic
}  // namespace Mav
