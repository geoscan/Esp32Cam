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
#include "utility/thr/WorkQueue.hpp"
#include <esp_log.h>

namespace Mav {
namespace Mic {

static constexpr const char *debugPreamble()
{
	return "FtpClient";
}

template <class Key>
static inline bool workqueuePushNotify(typename Key::template Arg<0> aPayload)
{
	if (!Ut::Thr::Wq::MediumPriority::checkInstance()) {
		ESP_LOGE(Mav::kDebugTag, "%s: Workqueue is unavailable, cannot notify", debugPreamble());

		return false;
	}

	Ut::Thr::Wq::MediumPriority::getInstance().push(
		[aPayload]()
		{
			Key::notify(aPayload);
		});

	return true;
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

static void logUnhandledOpcode(const char *aContext, const mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol);
static void logGotFailResponse(const char *aContext, const mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol);
static void logIncomingMessage(const mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol);

static inline void logUnhandledOpcode(const char *aContext,
	const mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol)
{
	ESP_LOGW(Mav::kDebugTag, "%s:%s: unexpected opcode pair opcode=%d req_opcode=%d", debugPreamble(), aContext,
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode),
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode));
}

static inline void logGotFailResponse(const char *aContext,
	const mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol)
{
	ESP_LOGW(Mav::kDebugTag, "%s:%s: got failure response to command req_opcode=%d opcode=%d error code=%d", debugPreamble(), aContext,
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode),
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode),
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().data[0]));
}

static void logIncomingMessage(const mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol)
{
	ESP_LOGV(Mav::kDebugTag, "%s: got incoming message req_opcode=%d opcode=%d error code=%d", debugPreamble(),
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode),
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode),
		static_cast<int>(static_cast<const Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().data[0]));
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
		logIncomingMessage(mavlinkFileTransferProtocol);

		return Ret::Ignored;
	}

	// Delegate further processing of the message
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};
	logIncomingMessage(mavlinkFileTransferProtocol);

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

		switch (requestRepeat.state) {
			case RequestRepeat::StateIdle:
			case RequestRepeat::StateCreatingSession:
			case RequestRepeat::StateClosingSession:
				requestRepeat.handleIdleTransition();

				break;

			case RequestRepeat::StateTransferring:
			case RequestRepeat::StateMax:
				// Make an attempt to close the session gracefully
				requestRepeat.handleClosingSessionTransition();

				break;
		}

		workqueuePushNotify<Bft::OnTransferUpdate>(Bft::TransferUpdateEvent().setFailed(
			"Exceeded max. number of re-send attempts"));
	} else {
		requestRepeat.handleFailedAttemptCommon();
	}

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			return;

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
			ESP_LOGI(Mav::kDebugTag, "%s:%s: Preparing transfer", debugPreamble(), __func__);
			requestRepeat.handleCreatingSessionTransition(aBftFile);
			resendSessionOpenRequest();
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
	ESP_LOGV(Mav::kDebugTag, "%s:%s resetting file position =%d", debugPreamble(), __func__,
		requestRepeat.stateTransferring.fileOffset);
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

inline Microservice::Ret FtpClient::processMavlinkMessageCreatingSession(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{

	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::OpenFileWo:
			stopTimer();

			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: { // Successfully opened

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
					logGotFailResponse(__func__, aMavlinkFileTransferProtocol);
					requestRepeat.handleIdleTransition();
					workqueuePushNotify<Bft::OnTransferUpdate>(Bft::TransferUpdateEvent().setFailed(
						"Could not create session, got FAILED response"));

					break;

				default:  // Should not get here, if the message was produced by the autopilot
					logUnhandledOpcode(__func__, aMavlinkFileTransferProtocol);

					break;
			}

			break;

		default:
			logUnhandledOpcode(__func__, aMavlinkFileTransferProtocol);
			break;
	}

	return Ret::NoResponse;
}

inline Microservice::Ret FtpClient::processMavlinkMessageTransferring(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::WriteFile:  // An attempt to write a chunk of a file
			stopTimer();

			// Read the next chunk
			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: {
					logTransferProgress();

					if (!validateIncomingMessageSessionId(aMavlinkFileTransferProtocol)) {
						return Ret::Ignored;
					}

					// Update the state
					requestRepeat.handleSuccessfulAttemptTransferring();

					if (requestRepeat.stateTransferringIsEof()) {
						ESP_LOGI(Mav::kDebugTag, "%s:%s successfully uploaded the file, closing session",
							debugPreamble(), __func__);
						requestRepeat.handleClosingSessionTransition();
					}

					// Pack the response
					initializeMavlinkMessage(aMavlinkMessage);

					// Trigger periodic re-sending of the request
					startOnce(kRequestResendTimeout);
					workqueuePushNotify<Bft::OnTransferUpdate>(Bft::TransferUpdateEvent().setInProgress(
						requestRepeat.stateTransferring.fileOffset, requestRepeat.stateTransferring.fileSize));

					aOnResponse(aMavlinkMessage);

					return Ret::Response;
				}
				case Ftp::Op::Nak:
					// Notify upon failure
					logGotFailResponse(__func__, aMavlinkFileTransferProtocol);
					workqueuePushNotify<Bft::OnTransferUpdate>(Bft::TransferUpdateEvent().setFailed(
						"Could not transfer a file chunk, got FAILED response"));

					// Instantiate session closing protocol
					requestRepeat.handleClosingSessionTransition();
					initializeMavlinkMessage(aMavlinkMessage);
					aOnResponse(aMavlinkMessage);

					// Start periodic resending
					startOnce(kRequestResendTimeout);

					// TODO: distinguish between "failure" and "success" session closing

					return Ret::Response;

				default:
					logUnhandledOpcode(__func__, aMavlinkFileTransferProtocol);

					break;
			}

			break;

		default:  // Not relevant -- so far, we only handle file writing-related responses here
			logUnhandledOpcode(__func__, aMavlinkFileTransferProtocol);

			break;
	}

	return Ret::Ignored;
}

inline Microservice::Ret FtpClient::processMavlinkMessageClosingSession(mavlink_message_t &aMavlinkMessage,
	mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol, Microservice::OnResponseSignature aOnResponse)
{
	switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().req_opcode) {
		case Ftp::Op::TerminateSession:
			stopTimer();

			switch (static_cast<Hlpr::FileTransferProtocol &>(aMavlinkFileTransferProtocol).getPayload().opcode) {
				case Ftp::Op::Ack: {
					if (validateIncomingMessageSessionId(aMavlinkFileTransferProtocol)) {
						workqueuePushNotify<Bft::OnTransferUpdate>(Bft::TransferUpdateEvent().setDone());
						ESP_LOGI(Mav::kDebugTag, "%s:%s successfully closed session, transferring to \"Idle\" state",
							debugPreamble(), __func__);

						// transfer to idle state
						requestRepeat.handleIdleTransition();
					}

					return Ret::NoResponse;
				}

				case Ftp::Op::Nak:
					logGotFailResponse(__func__, aMavlinkFileTransferProtocol);
					requestRepeat.handleIdleTransition();
					workqueuePushNotify<Bft::OnTransferUpdate>(Bft::TransferUpdateEvent().setFailed(
						"Could not close session, got FAILED response"));

					return Ret::NoResponse;

				default:
					logUnhandledOpcode(__func__, aMavlinkFileTransferProtocol);

					break;
			}

			break;

		default:  // Irrelevant
			logUnhandledOpcode(__func__, aMavlinkFileTransferProtocol);

			break;
	}

	return Ret::Ignored;
}

void FtpClient::initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage)
{
	(void)aMavlinkMessage;
	// TODO: change me
	static constexpr const char *kTemporaryFilePath = "/dev/LuaScript/main.lua";

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			break;

		case RequestRepeat::StateCreatingSession: {
			Mav::Hlpr::FileTransferProtocol mavlinkFileTransferProtocol{};
			mavlinkFileTransferProtocol.setOpenFileSessionFields(requestRepeat.stateCommon.messageSequenceNumber,
				Ftp::Op::OpenFileWo, kTemporaryFilePath, strlen(kTemporaryFilePath));
			mavlinkFileTransferProtocol.packInto(aMavlinkMessage);

			break;
		}
		case RequestRepeat::StateTransferring: {
			Mav::Hlpr::FileTransferProtocol mavlinkFileTransferProtocol{};

			// Initialize message
			mavlinkFileTransferProtocol.getPayload().size = requestRepeat.stateCommon.bftFile->read(
				reinterpret_cast<std::uint8_t *>(mavlinkFileTransferProtocol.getPayload().data),
				Mic::Ftp::Payload::kMaxDataLength);
			mavlinkFileTransferProtocol.setWriteFileFields(requestRepeat.stateCommon.messageSequenceNumber,
				requestRepeat.stateTransferring.mavlinkFtpSessionId,
				requestRepeat.stateTransferring.fileOffset);
			ESP_LOGV(Mav::kDebugTag, "%s:%s packed %d bytes", debugPreamble(), __func__,
				mavlinkFileTransferProtocol.getPayload().size);

			// Pack message
			mavlinkFileTransferProtocol.packInto(aMavlinkMessage);

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

inline void FtpClient::logTransferProgress()
{
	switch (requestRepeat.state) {
		case RequestRepeat::StateTransferring:
			ESP_LOGV(Mav::kDebugTag, "%s transferred %d/%d bytes", debugPreamble(),
				requestRepeat.stateTransferring.fileOffset, requestRepeat.stateTransferring.fileSize);

		default:
			break;
	};
}

inline FtpClient::RequestRepeat::RequestRepeat():
	stateCommon{0, {}, 0},
	mutex{},
	state{StateIdle}
{
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
	state = RequestRepeat::StateCreatingSession;
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
	ESP_LOGV(Mav::kDebugTag, "%s:%s Updated fileOffset=%d", debugPreamble(), __func__, stateTransferring.fileOffset);
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
