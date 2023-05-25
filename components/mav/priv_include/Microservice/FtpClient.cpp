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
		case RequestRepeat::StateIdle:
			return Ret::Ignored;

		case RequestRepeat::StateCreatingSession:
			return processMavlinkMessageCreatingSession(aMessage, mavlinkFileTransferProtocol, aOnResponse);

		case RequestRepeat::StateTransferring:
			return processMavlinkMessageTransferring(aMessage, mavlinkFileTransferProtocol, aOnResponse);

		case RequestRepeat::StateClosingSession:
			return processMavlinkMessageClosingSession(aMessage, mavlinkFileTransferProtocol, aOnResponse);

		case RequestRepeat::StateMax:
			return Ret::Ignored;
	}

	return Ret::Ignored;
}

void FtpClient::onHrTimer()
{
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};

	if (requestRepeat.stateCommon.iAttempt >= knMaxAttempts) {
		ESP_LOGE(Mav::kDebugTag, "%s:%s: state \"%s\": exceeded the number of attempts, cancelling transfer",
			debugPreamble(), __func__, requestRepeat.getCurrentStateAsString());
		requestRepeat.handleIdleTransition();
		// TODO: reset sessions, if any were opened

		// TODO: notify upon failure

		return;
	} else {
		requestRepeat.handleFailedAttemptCommon();
	}

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			break;

		case RequestRepeat::StateCreatingSession:
			resendSessionOpenRequest();

			break;

		case RequestRepeat::StateTransferring:
			resendFileTransferRequest();

			break;

		case RequestRepeat::StateClosingSession:
			resendSessionCloseRequest();

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
			requestRepeat.handleCreatingSessionTransition(aBftFile);
			resendSessionOpenRequest();  // TODO: XXX: from WQ?
			startOnce(kRequestResendTimeout);

			break;

		default:
			ESP_LOGW(Mav::kDebugTag, "%s:%s: already have an active session, ignoring", debugPreamble(), __func__);

			break;
	}
}

inline void FtpClient::resendSessionOpenRequest()
{
	DelayedSendAsyncCtx delayedSendAsyncCtx{
		Globals::getSysId(),
		Globals::getCompId(),
		DelayedSendAsyncVariant{static_cast<DelayedMavlinkMessageInitialization *>(this)}
	};
	notify(delayedSendAsyncCtx);
}

inline void FtpClient::resendFileTransferRequest()
{
	// Re-adjust the file's current position pointer
	requestRepeat.stateCommon.bftFile->seek(requestRepeat.stateTransferring.fileOffset);

	DelayedSendAsyncCtx delayedSendAsyncCtx{
		Globals::getSysId(),
		Globals::getCompId(),
		DelayedSendAsyncVariant{static_cast<DelayedMavlinkMessageInitialization *>(this)}
	};
	notify(delayedSendAsyncCtx);
}

inline void FtpClient::resendSessionCloseRequest()
{
	DelayedSendAsyncCtx delayedSendAsyncCtx{
		Globals::getSysId(),
		Globals::getCompId(),
		DelayedSendAsyncVariant{static_cast<DelayedMavlinkMessageInitialization *>(this)}
	};
	notify(delayedSendAsyncCtx);
}

// TODO: ensure that access to the encapsulated states is synchronized all accross the code

inline Microservice::Ret FtpClient::processMavlinkMessageCreatingSession(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{

	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::OpenFileWo:
			stopTimer();

			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: { // Successfully opened
					std::lock_guard<std::mutex> lock{requestRepeat.mutex};

					// Handle state transition
					requestRepeat.handleSuccessfulAttemptCreatingSession();
					requestRepeat.handleTransferringTransition(
						static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().session);

					ESP_LOGI(Mav::kDebugTag, "%s: %s successfully opened a file for writing session=%d",
						debugPreamble(), __func__, requestRepeat.stateTransferring.mavlinkFtpSessionId);

					// Initialize response message
					initializeMavlinkMessage(aMavlinkMessage);

					// Trigger re-sending the message
					startOnce(kRequestResendTimeout);

					aOnResponse(aMavlinkMessage);

					return Ret::Response;
				}

				case Ftp::Op::Nak:  // Failed to open
					requestRepeat.handleIdleTransition();
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

inline Microservice::Ret FtpClient::processMavlinkMessageTransferring(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	// TODO: close file at the end of transferring
	// TODO: instantiate session reset
	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::WriteFile:  // An attempt to write a chunk of a file
			stopTimer();

			// Read the next chunk
			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: {

					if (!validateIncomingMessageSessionId(aMavlinkFileTransferProtocol)) {
						return Ret::Ignored;
					}

					// Update the state
					std::lock_guard<std::mutex> lock{requestRepeat.mutex};
					requestRepeat.handleSuccessfulAttemptTransferring();

					if (requestRepeat.stateTransferringIsEof()) {
						requestRepeat.handleClosingSessionTransition();
					}

					// Pack the response
					initializeMavlinkMessage(aMavlinkMessage);

					// Trigger periodic re-sending of the request
					startOnce(kRequestResendTimeout);

					aOnResponse(aMavlinkMessage);

					return Ret::Response;
				}
				case Ftp::Op::Nak:
					ESP_LOGE(Mav::kDebugTag, "%s:%s failed to write file", debugPreamble(), __func__);
					requestRepeat.handleIdleTransition();

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

inline Microservice::Ret FtpClient::processMavlinkMessageClosingSession(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	(void)aMavlinkFileTransferProtocol;
	(void)aMavlinkMessage;
	(void)aOnResponse;

	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::TerminateSession:

			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: {
					if (validateIncomingMessageSessionId(aMavlinkFileTransferProtocol)) {
						// TODO: notify upon completion

						// transfer to idle state
						requestRepeat.handleIdleTransition();
					}

					return Ret::NoResponse;
				}

				case Ftp::Op::Nak:
					// TODO: handle failure
					// TODO: notify

					break;

				default:
					// TODO: Should not get here. Produce a warning
					// TODO: produce warning in other handlers too
					break;
			}

		default:  // Irrelevant
			break;
	}

	return Ret::Ignored;
}

void FtpClient::initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage)
{
	(void)aMavlinkMessage;
	static constexpr const char *kTemporaryFilePath = "/dev/UavMonitor/show.bin";

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			break;

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
			// TODO: copy data
			// TODO: pack into `aMavlinkMessage`

			break;
		}

		case RequestRepeat::StateClosingSession: {
			Mav::Hlpr::FileTransferProtocol mavlinkFileTransferProtocol{};
			mavlinkFileTransferProtocol.setTerminateSessionFields(requestRepeat.stateCommon.messageSequenceNumber,
				requestRepeat.stateClosingSession.mavlinkFtpSessionId);
			mavlinkFileTransferProtocol.packInto(aMavlinkMessage);

			break;
		}

		case RequestRepeat::StateMax:
			break;
	}
}

inline bool FtpClient::validateIncomingMessageSessionId(mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol)
{
	const auto sessionId = static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().session;

	if (sessionId != requestRepeat.stateClosingSession.mavlinkFtpSessionId) {
		ESP_LOGW(Mav::kDebugTag, "%s:%s: Got unexpected session id session=%d", debugPreamble(), __func__, sessionId);

		return false;
	}

	return true;
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

inline void FtpClient::RequestRepeat::handleIdleTransition()
{
	stateCommon.bftFile.reset();  // Reset ownership, the `shared_ptr`'s deleter will handle the rest
	state = RequestRepeat::StateIdle;
}

inline void FtpClient::RequestRepeat::handleTransferringTransition(std::size_t aMavlinkFtpSessionId)
{
	handleTransitionCommon();
	constexpr std::int32_t kInitialOffset = 0;
	stateCommon.bftFile->seek(0, Bft::FileSystem::PositionStart);  // Read from the beginning
	stateTransferring = {
		aMavlinkFtpSessionId,
		kInitialOffset, stateCommon.bftFile->getSize()
	};
	state = RequestRepeat::StateTransferring;
}

inline void FtpClient::RequestRepeat::handleCreatingSessionTransition(const std::shared_ptr<Bft::File> &aFile)
{
	handleTransitionCommon();
	stateCommon.bftFile = aFile;
}

inline void FtpClient::RequestRepeat::handleClosingSessionTransition()
{
	handleTransitionCommon();
	stateCommon.bftFile.reset();
	state = StateClosingSession;
	stateClosingSession = {stateTransferring.mavlinkFtpSessionId};
}

inline void FtpClient::RequestRepeat::handleTransitionCommon()
{
	stateCommon.iAttempt = 0;
}

inline void FtpClient::RequestRepeat::handleSuccessfulAttemptCommon()
{
	stateCommon.iAttempt = 0;
	++stateCommon.messageSequenceNumber;
}

inline void FtpClient::RequestRepeat::handleSuccessfulAttemptTransferring()
{
	handleSuccessfulAttemptCommon();
	stateTransferring.fileOffset = stateCommon.bftFile->getCurrentPosition();
}

inline void FtpClient::RequestRepeat::handleSuccessfulAttemptCreatingSession()
{
	handleSuccessfulAttemptCommon();
}

inline void FtpClient::RequestRepeat::handleFailedAttemptCommon()
{
	++stateCommon.iAttempt;
}

inline bool FtpClient::RequestRepeat::stateTransferringIsEof()
{
	return stateTransferring.fileOffset == stateTransferring.fileSize;
}

}  // namespace Mic
}  // namespace Mav
