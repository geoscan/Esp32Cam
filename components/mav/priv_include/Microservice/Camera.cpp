//
// Camera.cpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "Globals.hpp"
#include "Helper/CameraImageCaptured.hpp"
#include "Helper/Common.hpp"
#include "Helper/MavlinkCommandAck.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "Microservice/Camera.hpp"
#include "camera_recorder/RecFrame.hpp"
#include "mav/mav.hpp"
#include "module/ModuleBase.hpp"
#include "sd_fat.h"
#include "sub/Cam.hpp"
#include "sub/Rout.hpp"
#include "utility/LogSection.hpp"
#include "utility/time.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sdkconfig.h>

#define DEBUG_PRETEND_CAMERA_INITIALIZED 0

namespace Mav {
namespace Mic {

static constexpr const char *kLogPreamble = "Camera";

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

Camera::Camera() : HrTimer{ESP_TIMER_TASK, "MavHbeat", true}
{
	using namespace Mod;

	ModuleBase::Field<Module::Camera, Fld::Field::Initialized> fCameraInitialized;

	ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Initialized>(
		[&fCameraInitialized](bool aInit)
		{
			fCameraInitialized = aInit;
		});

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

						case MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED:
							ret = processRequestMessageCameraImageCaptured(commandLong, aMessage, aOnResponse);

							break;

						case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS:
							ret = processRequestMessageCameraCaptureStatus(commandLong, aMessage, aOnResponse);

							break;

						default:
							break;
					}

					break;

				case MAV_CMD_IMAGE_START_CAPTURE:
					ret = processCmdImageStartCapture(commandLong, aMessage, aOnResponse);

					break;

				case MAV_CMD_VIDEO_START_CAPTURE:
					ret = processCmdVideoStartCapture(commandLong, aMessage, aOnResponse);

					break;

				case MAV_CMD_VIDEO_STOP_CAPTURE:
					ret = processCmdVideoStopCapture(commandLong, aMessage, aOnResponse);

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
	using namespace Mod;
	ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation");

	ModuleBase::Field<Module::Camera, Fld::Field::Initialized> initialized;

	ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Initialized>(
		[&initialized](bool aInit)
		{
			initialized = aInit;
		});

#if DEBUG_PRETEND_CAMERA_INITIALIZED
	initialized = true;
#endif

	{
		auto mavlinkCommandAck = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMavlinkMessage, aMavlinkCommandLong.command,
			initialized ? MAV_RESULT_ACCEPTED : MAV_RESULT_DENIED);
		mavlinkCommandAck.packInto(aMavlinkMessage, Globals::getCompIdCamera());
		ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation - packing `COMMAND_ACK`");
		aOnResponse(aMavlinkMessage);
	}

#if DEBUG_PRETEND_CAMERA_INITIALIZED
		if (true) {
#else
		if (initialized) {
#endif
			mavlink_camera_information_t mavlinkCameraInformation {};
			mavlinkCameraInformation.time_boot_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::microseconds{Ut::bootTimeUs()}).count();
			mavlinkCameraInformation.flags = CAMERA_CAP_FLAGS_CAPTURE_IMAGE | CAMERA_CAP_FLAGS_CAPTURE_VIDEO |
				CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE | CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;

			ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::FrameSize>(
				[&mavlinkCameraInformation](ModuleBase::Field<Module::Camera, Fld::Field::FrameSize> aFs)
				{
					mavlinkCameraInformation.resolution_h = aFs.first;
					mavlinkCameraInformation.resolution_v = aFs.second;
				});

			ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::VendorName>(
				[&mavlinkCameraInformation](const char *aName)
				{
					static constexpr auto kVendorNameFieldLen = sizeof(mavlinkCameraInformation.vendor_name);
					ESP_LOGI(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation, got vendor name : %s",
						aName);
					std::copy_n(aName, std::min<int>(std::strlen(aName), kVendorNameFieldLen),
						mavlinkCameraInformation.vendor_name);
				});

			ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::ModelName>(
				[&mavlinkCameraInformation](const char *modelName)
				{
					ESP_LOGI(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation, got model name : %s",
						modelName);
					static constexpr auto kModelNameFieldLen = sizeof(mavlinkCameraInformation.model_name);
					std::copy_n(modelName, std::min<int>(std::strlen(modelName), kModelNameFieldLen),
						mavlinkCameraInformation.model_name);
				});

			mavlink_msg_camera_information_encode(Globals::getSysId(), Globals::getCompIdCamera(), &aMavlinkMessage,
				&mavlinkCameraInformation);
			ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraInformation - packing `CAMERA_INFORMATION`");
			aOnResponse(aMavlinkMessage);
	}

	return Microservice::Ret::Response;
}

Microservice::Ret Camera::processRequestMessageCameraImageCaptured(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	auto *prevCapture = history.findBySequence(static_cast<SequenceId>(aMavlinkCommandLong.param4));
	const auto requestedIndex = static_cast<int>(aMavlinkCommandLong.param2);

	ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraImageCaptured requestedIndex %d", requestedIndex);

	if (nullptr != prevCapture) {
		// Pack COMMAND_ACK
		{
			auto msg = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command,
				MAV_RESULT_ACCEPTED);
			msg.packInto(aMessage, Globals::getCompIdCamera());
			aOnResponse(aMessage);
		}
		// Pack CAMERA_IMAGE_CAPTURED
		{
			auto msg = Mav::Hlpr::CameraImageCaptured::make(prevCapture->sequence, prevCapture->result,
				prevCapture->imageName.c_str());

			msg.packInto(aMessage, Globals::getCompIdCamera());
			aOnResponse(aMessage);
		}
		ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraImageCaptured found the requested index. %d",
			requestedIndex);
	} else {  // `history` does not hold info on this capture. Probably, it never happened
		auto msg = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, MAV_RESULT_FAILED);
		msg.packInto(aMessage, Globals::getCompIdCamera());
		aOnResponse(aMessage);
		ESP_LOGW(Mav::kDebugTag, "Camera::processRequestMessageCameraImageCaptured could not find the requested"
			"index %d", requestedIndex);
	}

	return Ret::Response;
}

Microservice::Ret Camera::processRequestMessageCameraCaptureStatus(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	using namespace Mod;
	ESP_LOGD(Mav::kDebugTag, "Camera::processRequestMessageCameraCaptureStatus");
	// Pack and send `COMMAND_ACK`
	{
		auto ack = Mav::Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, MAV_RESULT_ACCEPTED);
		ack.packInto(aMessage, Globals::getCompIdCamera());
		aOnResponse(aMessage);
	}
	// Pack and send `CAMERA_CAPTURE_STATUS`
	{
		mavlink_camera_capture_status_t mavlinkCameraCaptureStatus {};
		Mav::Hlpr::Cmn::fieldTimeBootMsInit(mavlinkCameraCaptureStatus);
		mavlinkCameraCaptureStatus.image_count = history.imageCaptureCount;
		ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Recording>(
			[&mavlinkCameraCaptureStatus](bool aRecording) { mavlinkCameraCaptureStatus.video_status = aRecording; });
		mavlink_msg_camera_capture_status_encode(Globals::getSysId(), Globals::getCompIdCamera(), &aMessage,
			&mavlinkCameraCaptureStatus);
		aOnResponse(aMessage);
	}

	return Ret::Response;
}

Microservice::Ret Camera::processCmdImageStartCapture(mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	GS_UTILITY_LOG_SECTIOND(Mav::kDebugTag, "Camera::processCmdImageStartCapture");

	auto imageCapture = processMakeShot(aMavlinkCommandLong);

	// Send ACK w/ value depending on whether or not the capture was successful
	{
		ESP_LOGD(Mav::kDebugTag, "Camera::processCmdImageStartCapture, packing ACK");
		auto mavlinkCommandAck = Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command,
			imageCapture.result);
		mavlinkCommandAck.packInto(aMessage, Globals::getCompIdCamera());
		aOnResponse(aMessage);
	}

	if (MAV_RESULT_ACCEPTED == imageCapture.result) {
		ESP_LOGD(Mav::kDebugTag, "Camera::processCmdImageStartCapture, packing IMAGE_CAPTURED");
		auto mavlinkCameraImageCaptured = Mav::Hlpr::CameraImageCaptured::make(imageCapture.sequence,
			imageCapture.result, imageCapture.imageName.c_str());
		mavlinkCameraImageCaptured.packInto(aMessage, Globals::getCompIdCamera());
		aOnResponse(aMessage);
	}

	history.imageCaptureSequence.push_back(imageCapture);  // Push capture info into history
	history.imageCaptureCount += imageCapture.result;

	return Ret::Response;
}

Microservice::Ret Camera::processCmdVideoStartCapture(const mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature &aOnResponse)
{
	sdFatInit();
	using namespace Mod;
	GS_UTILITY_LOG_SECTIOND(Mav::kDebugTag, "Camera::processCmdVideoStartCapture");

	if (static_cast<int>(aMavlinkCommandLong.param2) != 0) {  // Current implementation does not support sending periodic CAMERA_CAPTURE_STATUS during capture
		static constexpr auto kResult = MAV_RESULT_DENIED;
		ESP_LOGW(Mav::kDebugTag, "Camera::processCmdVideoStartCapture result %d "
			"COMMAND_LONG.param2=%d", kResult, static_cast<int>(aMavlinkCommandLong.param2));
		Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
			.packInto(aMessage, Globals::getCompIdCamera());
		aOnResponse(aMessage);
	} else {
		bool initialized = false;

		ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Initialized>(
			[&initialized] (bool aInitialized)
			{
				initialized = aInitialized;
			});

		if (initialized) {
			bool recording = false;

			ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Recording>(
				[&recording](bool aStatus)
				{
					recording = aStatus;
				});

			if (!recording) {
				static constexpr std::size_t kNameMaxLen = 6;
				char filename[kNameMaxLen] = {0};
				++history.imageCaptureCount;

				ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::CaptureCount>(
					[&filename](unsigned aCount)
					{
						ESP_LOGD(Mav::kDebugTag, "Camera::processCmdVideoStartCapture got CaptureCount from camera %d",
							aCount);
						snprintf(filename, kNameMaxLen, "%d", aCount);
						filename[kNameMaxLen - 1] = 0;
					});

				Sub::Cam::RecordStart::notify(filename);
				static constexpr auto kResult = MAV_RESULT_ACCEPTED;
				ESP_LOGI(Mav::kDebugTag, "Camera::processCmdVideoStartCapture result %d", kResult);
				Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
					.packInto(aMessage, Globals::getCompIdCamera());
				aOnResponse(aMessage);
			} else {
				static constexpr auto kResult = MAV_RESULT_DENIED;
				ESP_LOGW(Mav::kDebugTag, "Camera::processCmdVideoStartCapture result %d already recording", kResult);
				Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
					.packInto(aMessage, Globals::getCompIdCamera());
				aOnResponse(aMessage);
			}
		} else {
			static constexpr auto kResult = MAV_RESULT_FAILED;
			ESP_LOGE(Mav::kDebugTag, "Camera::processCmdVideoStartCapture result %d camera not initialized", kResult);

			Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
				.packInto(aMessage, Globals::getCompIdCamera());
			aOnResponse(aMessage);
		}
	}

	return Ret::Response;
}

Microservice::Ret Camera::processCmdVideoStopCapture(const mavlink_command_long_t &aMavlinkCommandLong,
	mavlink_message_t &aMessage, Microservice::OnResponseSignature &aOnResponse)
{
	using namespace Mod;
	GS_UTILITY_LOG_SECTIOND(Mav::kDebugTag, "Camera::processCmdVideoStopCapture");
	bool initialized = false;

#if DEBUG_PRETEND_CAMERA_INITIALIZED
	initialized = true;
#else
	ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Initialized>(
		[&initialized](bool aInitialized) {initialized = aInitialized; });
#endif

	if (initialized) {
		bool recording = false;

		ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::Recording>(
			[&recording](bool aRecording) {recording = aRecording; });

		if (recording) {
			Sub::Cam::RecordStop::notify();
			static constexpr auto kResult = MAV_RESULT_ACCEPTED;
			ESP_LOGI(Mav::kDebugTag, "Camera::processCmdVideoStopCapture result %d", kResult);
			Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
				.packInto(aMessage, Globals::getCompIdCamera());
			aOnResponse(aMessage);
		} else {
			static constexpr auto kResult = MAV_RESULT_DENIED;
			ESP_LOGE(Mav::kDebugTag, "Camera::processCmdVideoStopCapture result %d no ongoing recording", kResult);
			Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
				.packInto(aMessage, Globals::getCompIdCamera());
			aOnResponse(aMessage);
		}
	} else {
		static constexpr auto kResult = MAV_RESULT_FAILED;
		ESP_LOGE(Mav::kDebugTag, "Camera::processCmdVideoStopCapture result %d camera not initialized", kResult);
		Hlpr::MavlinkCommandAck::makeFrom(aMessage, aMavlinkCommandLong.command, kResult)
			.packInto(aMessage, Globals::getCompIdCamera());
		aOnResponse(aMessage);
	}

	return Ret::Response;
}

Camera::ImageCapture Camera::processMakeShot(const mavlink_command_long_t &aMavlinkCommandLong)
{
	using namespace Mod;

	ImageCapture imageCapture {static_cast<SequenceId>(aMavlinkCommandLong.param4),  // Sequence id. of the capture
		static_cast<unsigned>(aMavlinkCommandLong.param3),  // Number of images, total
		MAV_RESULT_DENIED,  // Result of the capture
		""};
	sdFatInit();

	if (1 == imageCapture.totalImages) {
		const auto *prevCapture = history.findBySequence(imageCapture.sequence);
		const bool shouldCapture = nullptr != prevCapture || 0 == aMavlinkCommandLong.confirmation;

		ESP_LOGD(Mav::kDebugTag, "Camera::processMakeShot, has prev. capture %s first request %s, confirmation %d",
			nullptr == prevCapture ? "FALSE" : "TRUE", 0 == aMavlinkCommandLong.confirmation ? "TRUE" : "FALSE",
			aMavlinkCommandLong.confirmation);

		if (shouldCapture) {
			static constexpr std::size_t kNameMaxLen = 6;
			char filename[kNameMaxLen] = {0};
			++history.imageCaptureCount;

			ModuleBase::moduleFieldReadIter<Module::Camera, Fld::Field::CaptureCount>(
				[&filename](unsigned aCount)
				{
					ESP_LOGD(Mav::kDebugTag, "Camera::processMakeShot got CaptureCount from camera %d",
						aCount);
					snprintf(filename, kNameMaxLen, "%d", aCount);
				});

			if (CameraRecorder::RecFrame::checkInstance()) {
				if (CameraRecorder::RecFrame::getInstance().start(filename)) {
					imageCapture.result = MAV_RESULT_ACCEPTED;
				} else {
					imageCapture.result = MAV_RESULT_FAILED;
					ESP_LOGE(Mav::kDebugTag, "%s::%s failed to make a capture", kLogPreamble, __func__);
				}
			} else {
				ESP_LOGE(Mav::kDebugTag, "%s::%s frame capturer instance is not initialized, cannot proceed",
					kLogPreamble, __func__);
				imageCapture.result = MAV_RESULT_DENIED;
			}

			ESP_LOGI(Mav::kDebugTag, "Camera::processMakeShot, request to make a shot, frame name \"%s\" "
				"response code \"%d\"", filename, imageCapture.result);

			history.imageCaptureSequence.push_back(imageCapture);
		}
	} else {
		imageCapture.result = MAV_RESULT_UNSUPPORTED;

		ESP_LOGW(Mav::kDebugTag, "Current implementation does not support periodic shots");
	}

	return imageCapture;
}

Camera::ImageCapture *Camera::History::findBySequence(Camera::SequenceId aSequence)
{
	auto it = std::find_if(imageCaptureSequence.begin(), imageCaptureSequence.end(),
		[aSequence](const ImageCapture &aImageCapture) {return aImageCapture.sequence == aSequence;});

	return imageCaptureSequence.end() == it ? nullptr : &*it;
}

}  // namespace Mic
}  // namespace Mav
