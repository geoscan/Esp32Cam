//
// FtpClient.hpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_MAV_PRIV_INCLUDE_MICROSERVICE_FTPCLIENT_HPP_
#define COMPONENTS_MAV_PRIV_INCLUDE_MICROSERVICE_FTPCLIENT_HPP_

#include "DelayedMavlinkMessageInitialization.hpp"
#include "DelayedSend.hpp"
#include "Microservice.hpp"
#include "buffered_file_transfer/Sub.hpp"
#include "buffered_file_transfer/storage/File.hpp"
#include "utility/system/HrTimer.hpp"
#include <mutex>

namespace Mav {
namespace Mic {

/// \brief Implements client side of MAVLink File Transfer Protocol for
/// flashing file into AP's FS.
class FtpClient final : public Microservice, public DelayedSend, public Ut::Sys::HrTimer,
	public DelayedMavlinkMessageInitialization {
private:
	struct SubscriptionPackage {
		Bft::OnFileBufferingFinished onFileBufferingFinished;
	};

	/// \brief Keeps track of the previous attempts to send a file chunk to the
	/// AP
	struct RequestRepeat {
		enum State {
			StateIdle = 0,
			StateCreatingSession,
			StateTransferring,

			StateMax,
		};

		union {
			struct {
				std::size_t sessionId;
			} stateCreatingSession;

			struct {
				std::size_t mavlinkFtpSessionId;
				std::int32_t fileOffset;
				std::uint32_t fileSize;
			} stateTransferring;
		};

		/// \brief Entities used by more than 1 state
		struct {
			// Ensure it is reset
			std::size_t iAttempt;
			std::shared_ptr<Bft::File> bftFile;
			// TODO: increment on state transitions
			// TODO: initialize in constructor,
			std::uint16_t messageSequenceNumber;
			// TODO: lock current file position
		} stateCommon;

		std::mutex mutex;
		State state;

		static const char *stateAsString(State aState);
		const char *getCurrentStateAsString();

		/// \brief Releases the resources, sets the new state
		void handleIdleTransition();

		/// \pre `state == StateCreatingSession`
		void handleTransferringTransition(std::size_t aMavlinkFtpSessionId);

		/// \pre `state == StateIdle`
		void handleCreatingSessionTransition(const std::shared_ptr<Bft::File> &aFile);

		/// \brief Performs routines that are common for ALL states
		void handleTransitionCommon();

		void handleSuccessfulAttemptCommon();
		void handleSuccessfulAttemptTransferring();
		void handleSuccessfulAttemptCreatingSession();
		void handleFailedAttemptCommon();

		bool stateTransferringIsEof();

		// TODO: file position
		// TOOD: BFT and HrTimer events come at different times (XXX :will the mutex suffice?)
	};

public:
	FtpClient();
	~FtpClient();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;

	/// \brief Re-sends a message depending on the current state.
	void onHrTimer() override;

private:
	/// \brief Handles an "end-of-buffering" event
	/// \param `aBftFile` is a RAII file wrapper. It is supposed to
	/// automiatically close the file.
	void onFileBufferingFinished(std::shared_ptr<::Bft::File> aBftFile);

	/// \brief Delegate for `onHrTimer()` and `onFileBufferingFinished(...)`.
	/// Handles re-sending of the message.
	void resendSessionOpenRequest();

	/// \brief Delegate for `onHrTimer()`. Handles re-sending of the message
	void resendFileTransferRequest();

	/// \brief delegate for `process(...)`
	Ret processMavlinkMessageCreatingSession(mavlink_message_t &aMavlinkMessage,
		mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol,
		Microservice::OnResponseSignature aOnResponse);

	/// \brief delegate for `process(...)`
	Ret processMavlinkMessageTransferring(mavlink_message_t &aMavlinkMessage,
		mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol,
		Microservice::OnResponseSignature aOnResponse);

	// TODO: check for multiple mutex locks
	/// \brief Initializes `aMavlinkMessage` depending on the current state.
	/// Designed to get invoked on a re-attempt, but it is also useful for
	/// initializing a response
	void initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage) override;

private:
	SubscriptionPackage subscriptionPackage;
	RequestRepeat requestRepeat;
};

}  // namespace Mic
}  // namespace Mav

#endif // FTPCLIENT_HPP_
