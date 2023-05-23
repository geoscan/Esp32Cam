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
				std::size_t bftFilePosition;
				std::size_t mavlinkFtpSessionId;
			} stateTransferring;
		};

		/// \brief Entities used by more than 1 state
		struct {
			std::size_t iAttempt;
			std::shared_ptr<Bft::File> bftFile;
		} stateCommon;

		std::mutex mutex;
		State state;

		static const char *stateAsString(State aState);
		const char *currentStateAsString();
		// TODO: file position
		// TOOD: BFT and HrTimer events come at different times (XXX :will the mutex suffice?)
	};

public:
	FtpClient();
	~FtpClient();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;

	/// \brief Re-instantiates a request
	void onHrTimer() override;

private:
	/// \brief Handles an "end-of-buffering" event
	/// \param `aBftFile` is a RAII file wrapper. It is supposed to
	/// automiatically close the file.
	void onFileBufferingFinished(std::shared_ptr<::Bft::File> aBftFile);

	void sendSessionOpenRequest();
	void sendFileTransferRequest();
	Ret processMavlinkMessageCreatingSession(mavlink_message_t &aMavlinkMessage,
		mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol,
		Microservice::OnResponseSignature aOnResponse);
	Ret processMavlinkMessageTransferring(mavlink_message_t &aMavlinkMessage,
		mavlink_file_transfer_protocol_t &aMavlinkFileTransferProtocol,
		Microservice::OnResponseSignature aOnResponse);
	void initializeMavlinkMessage(mavlink_message_t &aMavlinkMessage) override;

private:
	SubscriptionPackage subscriptionPackage;
	RequestRepeat requestRepeat;
};

}  // namespace Mic
}  // namespace Mav

#endif // FTPCLIENT_HPP_
