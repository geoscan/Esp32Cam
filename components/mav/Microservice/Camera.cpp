//
// Camera.cpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "Microservice/Camera.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "Helper/MavlinkCommandAck.hpp"
#include "Helper/Common.hpp"
#include "sub/Rout.hpp"
#include "sub/Cam.hpp"
#include "mav/mav.hpp"
#include "Globals.hpp"
#include "sub/Sys.hpp"
#include "utility/time.hpp"
#include "sd_fat.h"
#include <cstring>
#include <algorithm>
#include <chrono>

#define DEBUG_PRETEND_CAMERA_INITIALIZED 1

namespace Mav {
namespace Mic {

void Camera::onHrTimer()
{
	mavlink_heartbeat_t mavlinkHeartbeat;
	mavlinkHeartbeat.type = MAV_TYPE_CAMERA;
	mavlinkHeartbeat.autopilot = MAV_AUTOPILOT_INVALID;
	mavlinkHeartbeat.base_mode = 0;
	mavlinkHeartbeat.custom_mode = 0;
	mavlinkHeartbeat.system_status = MAV_STATE_STANDBY;

	mavlink_message_t mavlinkMessage;
	mavlink_msg_heartbeat_encode(Globals::getSysId(), Globals::getCompIdCamera(), &mavlinkMessage, &mavlinkHeartbeat);
	notify(mavlinkMessage);
}

Camera::Camera() : HrTimer{ESP_TIMER_TASK, "MavHbeat"}
{
	using namespace Sub::Sys;

	bool fCameraInitialized = false;

	for (auto &cb : Fld::ModuleGetField::getIterators()) {
		auto resp = cb(Fld::Req{Module::Camera, Fld::Field::Initialized});

		if (resp.tryGet<Module::Camera, Fld::Field::Initialized>(fCameraInitialized)) {
			ESP_LOGI(Mav::kDebugTag, "Microservice::Camera: Heartbeat emit, found camera module");
		}
	}

#if DEBUG_PRETEND_CAMERA_INITIALIZED
	fCameraInitialized = true;
#endif

	if (fCameraInitialized) {
		startPeriodic(std::chrono::seconds(1));   // 1 Hz, https://mavlink.io/en/services/camera.html#camera-connection
	} else {
		ESP_LOGW(Mav::kDebugTag, "Microservice::Camera: Heartbeat emit, camera module was not initialized");
	}
}

Microservice::Ret Camera::process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse)
{
	Microservice::Ret ret = Microservice::Ret::Ignored;

	switch (aMessage.msgid) {
		case MAVLINK_MSG_ID_COMMAND_LONG:  // Falls through
		case MAVLINK_MSG_ID_COMMAND_INT: {

			mavlink_command_long_t commandLong = Hlpr::MavlinkCommandLong::makeFrom(aMessage);

			switch (commandLong.command) {

				case MAV_CMD_REQUEST_MESSAGE:
					switch (static_cast<int>(commandLong.param1)) {
						case MAVLINK_MSG_ID_CAMERA_INFORMATION:
							ret = processRequestMessageCameraInformation(commandLong, aMessage, aOnResponse);

							break;

						default:
							break;
					}

					break;

				case MAV_CMD_IMAGE_START_CAPTURE:

					ret = processCmdImageStartCapture(commandLong, aMessage, aOnResponse);

					break;

				default:
					break;
			}
		}

		default:
			break;
	}

	return ret;
}

Microservice::Ret Camera::processRequestMessageCameraInformation(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMavlinkMessage, Microservice::OnResponseSignature aOnResponse)
{
	assert(static_cast<int>(aMavlinkCommandLong.param1) == MAVLINK_MSG_ID_CAMERA_INFORMATION);
	using namespace Sub::Sys;
	ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation");

	struct {
		int sysid;
		int compid;
	} sender {aMavlinkMessage.sysid, aMavlinkMessage.compid};

	typename Fld::GetType<Fld::Field::Initialized, Module::Camera>::Type initialized = false;

	for (auto &cb : Fld::ModuleGetField::getIterators()) {
		Fld::moduleCbTryGet<Module::Camera, Fld::Field::Initialized>(cb, initialized);
	}

#if DEBUG_PRETEND_CAMERA_INITIALIZED
	initialized = true;
#endif

	{
		mavlink_command_ack_t mavlinkCommandAck {};
		mavlinkCommandAck.target_component = sender.compid;
		mavlinkCommandAck.target_system = sender.sysid;
		mavlinkCommandAck.result = initialized ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED;
		mavlinkCommandAck.command = aMavlinkCommandLong.command;

		mavlink_msg_command_ack_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage, &mavlinkCommandAck);
		ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation - packing `COMMAND_ACK`");
		aOnResponse(aMavlinkMessage);
	}

	if (initialized) {
		mavlink_camera_information_t mavlinkCameraInformation {};
		mavlinkCameraInformation.time_boot_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::microseconds{Utility::bootTimeUs()}).count();
		mavlinkCameraInformation.flags = CAMERA_CAP_FLAGS_CAPTURE_IMAGE | CAMERA_CAP_FLAGS_CAPTURE_VIDEO |
			CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE | CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;

		for (auto &cb : Fld::ModuleGetField::getIterators()) {
#if DEBUG_PRETEND_CAMERA_INITIALIZED
			if (true) {
#else
			if (Fld::moduleCbTryGet<Module::Camera, Fld::Field::Initialized>(cb, initialized)) {
#endif
				ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation. Packing fields");
				{
					typename Fld::GetType<Fld::Field::FrameSize, Module::Camera>::Type frameSize{};

					if (Fld::moduleCbTryGet<Module::Camera, Fld::Field::FrameSize>(cb, frameSize)) {
						ESP_LOGI(Mav::kDebugTag, "Camera frame size %dx%d", frameSize.first, frameSize.second);
						mavlinkCameraInformation.resolution_h = frameSize.first;
						mavlinkCameraInformation.resolution_v = frameSize.second;
					}
				}
				{
					typename Fld::GetType<Fld::Field::VendorName, Module::Camera>::Type vendorName;

					if (Fld::moduleCbTryGet<Module::Camera, Fld::Field::VendorName>(cb, vendorName)) {
						static constexpr auto kVendorNameFieldLen = sizeof(mavlinkCameraInformation.vendor_name);
						ESP_LOGI(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation, got vendor name : %s",
							vendorName);
						std::copy_n(vendorName, std::min<int>(std::strlen(vendorName), kVendorNameFieldLen),
							mavlinkCameraInformation.vendor_name);
					}
				}
				{
					typename Fld::GetType<Fld::Field::ModelName, Module::Camera>::Type modelName;

					if (Fld::moduleCbTryGet<Module::Camera, Fld::Field::ModelName>(cb, modelName)) {
						ESP_LOGI(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation, got model name : %s",
							modelName);
						static constexpr auto kModelNameFieldLen = sizeof(mavlinkCameraInformation.model_name);
						std::copy_n(modelName, std::min<int>(std::strlen(modelName), kModelNameFieldLen),
							mavlinkCameraInformation.model_name);
					}
				}
			}
		}

		mavlink_msg_camera_information_encode(Globals::getSysId(), Globals::getCompId(), &aMavlinkMessage,
			&mavlinkCameraInformation);
		ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation - packing `CAMERA_INFORMATION`");
		aOnResponse(aMavlinkMessage);
	}

	return Microservice::Ret::Response;
}

Microservice::Ret Camera::processCmdImageStartCapture(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	sdFatInit();
	MAV_RESULT mavResult = MAV_RESULT_FAILED;
	static constexpr std::size_t kNameMaxLen = 6;
	char filename[kNameMaxLen] = {0};

	if (static_cast<int>(aMavlinkCommandLong.param3) != 1) {  // Number of total images should be eq. 1
		mavResult = MAV_RESULT_UNSUPPORTED;
		ESP_LOGW(Mav::kDebugTag, "Camera::processCmdImageStartCapture Periodic shoots are not supported");
	}

	// Auto-generate name
	if (MAV_RESULT_UNSUPPORTED != mavResult) {
		std::uint16_t stamp = static_cast<uint16_t>(Utility::bootTimeUs() & 0xffff);
		snprintf(filename, kNameMaxLen, "%d", stamp);

		for (auto &cb : Sub::Cam::ShotFile::getIterators()) {
			mavResult = cb(filename) ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED;
		}
	}

	ESP_LOGI(Mav::kDebugTag, "Camera::processCmdImageStartCapture, request to make a shot, frame name \"%s\""
		"response code \"%d\"", filename, mavResult);

	// Send ACK w/ value depending on whether or not the capture was successful
	{
		auto mavlinkCommandAck = Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, mavResult);
		mavlinkCommandAck.packInto(aMessage);
		aOnResponse(aMessage);
	}

	if (MAV_RESULT_ACCEPTED == mavResult) {
		mavlink_camera_image_captured_t mavlinkCameraImageCaptured {};
		Hlpr::Cmn::fieldTimeBootMsInit(mavlinkCameraImageCaptured);
		mavlinkCameraImageCaptured.image_index = 0;
		std::copy_n(filename, std::min<int>(kNameMaxLen, sizeof(mavlinkCameraImageCaptured.file_url)),
			mavlinkCameraImageCaptured.file_url);

		mavlink_msg_camera_image_captured_encode(Globals::getSysId(), Globals::getCompId(), &aMessage,
			&mavlinkCameraImageCaptured);
		aOnResponse(aMessage);
	}

	return Ret::Response;
}

}  // namespace Mic
}  // namespace Mav
