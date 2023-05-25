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

// TODO: ensure that access to the encapsulated states is synchronized all accross the code

Microservice::Ret FtpClient::processMavlinkMessageCreatingSession(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};

	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::OpenFileWo:  // TODO: ensure consistency, that this is the exact command we've sent (in the other part of the code)
			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack:  // Successfully opened
					// Initialize the state
					requestRepeat.stateCommon.iAttempt = 0;
					requestRepeat.stateCommon.bftFile->seek(0, Bft::FileSystem::PositionStart);  // Reed from the beginning
					requestRepeat.state = RequestRepeat::StateTransferring;
					requestRepeat.stateTransferring = {
						static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().session};
					++requestRepeat.stateCommon.messageSequenceNumber;

					ESP_LOGI(Mav::kDebugTag, "%s: %s successfully opened a file for writing session=%d",
						debugPreamble(), __func__, requestRepeat.stateTransferring.mavlinkFtpSessionId);
					break;

				case Ftp::Op::Nak:  // Failed to open
					requestRepeat.stateCommon.bftFile.reset();  // Reset ownership, the `shared_ptr`'s deleted will handle the rest
					ESP_LOGE(Mav::kDebugTag, "%s:%s failed to create session", debugPreamble(), __func__);

					break;

				default:  // Should not get here, if the message was produced by the autopilot
					break;
			}

			break;

		default:
			break;
	}

	return Ret::NoResponse;
}

Microservice::Ret FtpClient::processMavlinkMessageTransferring(mavlink_message_t &aMavlinkMessage, mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	// TODO: close file at the end of transferring
	// TODO: instantiate session reset
	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::WriteFile:  // An attempt to write a chunk of a file
			stopTimer();
			// Read the next chunk
			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: {
					// Update the state
					std::lock_guard<std::mutex> lock{requestRepeat.mutex};
					requestRepeat.stateCommon.iAttempt = 0;
					++requestRepeat.stateCommon.messageSequenceNumber;

					// Pack the response
					initializeMavlinkMessage(aMavlinkMessage);

					// Trigger periodic re-sending of the request
					startOnce(kRequestResendTimeout);

					return Ret::Response;
				}
				case Ftp::Op::Nak:
					ESP_LOGE(Mav::kDebugTag, "%s:%s failed to write file", debugPreamble(), __func__);

					break;

				default:
					break;
			}

			break;

		default:  // Not relevant -- so far, we only handle file writing-related responses here
			break;
	}

	return Ret::Ignored;
}

void FtpClient::initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage)
{
	(void)aMavlinkMessage;
	static constexpr const char *kTemporaryFilePath = "/dev/UavMonitor/show.bin";

	switch (requestRepeat.state) {
		case RequestRepeat::StateCreatingSession: {
			Mav::Hlpr::FileTransferProtocol mavlinkFileTransferProtocol{};
			mavlinkFileTransferProtocol.setOpenFileSessionFields(requestRepeat.stateCommon.messageSequenceNumber,
				Ftp::Op::OpenFileWo, kTemporaryFilePath, strlen(kTemporaryFilePath));
			mavlinkFileTransferProtocol.packInto(aMavlinkMessage);
			// TODO. XXX. Anything else?

			break;
		}
		case RequestRepeat::StateTransferring: {
			Mav::Hlpr::FileTransferProtocol mavlinkFileTransferProtocol{};
			// TODO: in other handler, ensure that the file has its position set
			mavlinkFileTransferProtocol.getPayload().size = requestRepeat.stateCommon.bftFile->read(
				reinterpret_cast<std::uint8_t *>(mavlinkFileTransferProtocol.getPayload().data),
				Mic::Ftp::Payload::kMaxDataLength);
			mavlinkFileTransferProtocol.setWriteFileFields(requestRepeat.stateCommon.messageSequenceNumber,
				requestRepeat.stateTransferring.mavlinkFtpSessionId,
				requestRepeat.stateCommon.bftFile->getCurrentPosition());

			break;
		}
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
