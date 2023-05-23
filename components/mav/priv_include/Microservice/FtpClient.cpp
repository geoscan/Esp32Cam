//
// FtpClient.cpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "FtpClient.hpp"
#include "Globals.hpp"
#include "Helper/FileTransferProtocol.hpp"
#include "Microservice/Ftp/Types.hpp"
#include "mav/mav.hpp"
#include <esp_log.h>

namespace Mav {
namespace Mic {

static constexpr const char *debugPreamble()
{
	return "FtpClient";
}

static constexpr std::chrono::milliseconds kRequestResendTimeout{200};
static constexpr std::size_t knMaxAttempts = 4;

FtpClient::FtpClient():
	Ut::Sys::HrTimer{ESP_TIMER_TASK, "MavFtpClient"},
	subscriptionPackage{
		{&FtpClient::onFileBufferingFinished, this}  // onFileBufferingFinished
	}
{
}

FtpClient::~FtpClient()
{
	subscriptionPackage.onFileBufferingFinished.setEnabled(false);
}

Microservice::Ret FtpClient::process(mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	// Perform initial filtering-out by message and target system identifiers

	if (aMessage.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
		return Ret::Ignored;
	}

	mavlink_file_transfer_protocol_t mavlinkFileTransferProtocol{};
	mavlink_msg_file_transfer_protocol_decode(&aMessage, &mavlinkFileTransferProtocol);

	if (mavlinkFileTransferProtocol.target_component != Globals::getCompId() ||
			mavlinkFileTransferProtocol.target_system != Globals::getSysId()) {
		ESP_LOGD(Mav::kDebugTag, "%s:%s: dropping an FTP message with non-matching target target_system=%d"
			"target_component=%d", debugPreamble(), __func__, mavlinkFileTransferProtocol.target_system,
			mavlinkFileTransferProtocol.target_component);
		return Ret::Ignored;
	}

	// Delegate further processing of the message

	switch (requestRepeat.state) {
		case RequestRepeat::StateCreatingSession:
			return processMavlinkMessageCreatingSession(aMessage, mavlinkFileTransferProtocol, aOnResponse);

		case RequestRepeat::StateTransferring:
			return processMavlinkMessageTransferring(aMessage, mavlinkFileTransferProtocol, aOnResponse);

		default:
			return Ret::Ignored;
	}
}

void FtpClient::onHrTimer()
{
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};

	if (requestRepeat.stateCommon.iAttempt >= knMaxAttempts) {
		ESP_LOGE(Mav::kDebugTag, "%s:%s: state \"%s\": exceeded the number of attempts, cancelling transfer",
			debugPreamble(), __func__, requestRepeat.getCurrentStateAsString());

		// TODO: notify upon failure
	} else {
		++requestRepeat.stateCommon.iAttempt;
	}

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			break;

		case RequestRepeat::StateCreatingSession:
			sendSessionOpenRequest();

			break;

		case RequestRepeat::StateTransferring:
			sendFileTransferRequest();

			break;

		case RequestRepeat::StateMax:
			break;
	}

	startOnce(kRequestResendTimeout);
}

void FtpClient::onFileBufferingFinished(std::shared_ptr<::Bft::File> aBftFile)
{
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			requestRepeat.stateCommon = {0, aBftFile, 0};
			sendSessionOpenRequest();  // TODO: XXX: from WQ?
			startOnce(kRequestResendTimeout);

			break;

		default:
			ESP_LOGW(Mav::kDebugTag, "%s:%s: already have an active session, ignoring", debugPreamble(), __func__);

			break;
	}
}

void FtpClient::sendSessionOpenRequest()
{
	// TODO
}

void FtpClient::sendFileTransferRequest()
{
	// TODO
}

Microservice::Ret FtpClient::processMavlinkMessageCreatingSession(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	// TODO:
	(void)aMavlinkFileTransferProtocol;
	(void)aMavlinkMessage;
	(void)aOnResponse;

	return Ret::Ignored;
}

Microservice::Ret FtpClient::processMavlinkMessageTransferring(mavlink_message_t &aMavlinkMessage, mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	// TODO
	(void)aMavlinkFileTransferProtocol;
	(void)aMavlinkMessage;
	(void)aOnResponse;

	return Ret::Ignored;
}

void FtpClient::initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage)
{
	(void)aMavlinkMessage;
	// TODO

	switch (requestRepeat.state) {
		case RequestRepeat::StateCreatingSession: {
			Mav::Hlpr::FileTransferProtocol mavlinkFileTransferProtocol{};
			mavlinkFileTransferProtocol.setOpenFileSessionFields(requestRepeat.stateCommon.messageSequenceNumber,
				Ftp::Op::OpenFileWo);
			mavlinkFileTransferProtocol.packInto(aMavlinkMessage);
			// TODO. XXX. Anything else?

			break;
		}
		case RequestRepeat::StateTransferring:
			// TODO
			break;

		default:
			break;
	}
}

inline const char *FtpClient::RequestRepeat::stateAsString(FtpClient::RequestRepeat::State aState)
{
	static constexpr const char *kStateNameMapping[] = {
		"Idle",
		"Creating session",
		"Transferring"
	};

	return kStateNameMapping[aState];
}

inline const char *FtpClient::RequestRepeat::getCurrentStateAsString()
{
	return stateAsString(state);
}

}  // namespace Mic
}  // namespace Mav
