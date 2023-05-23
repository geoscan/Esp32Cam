//
// FtpClient.cpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "FtpClient.hpp"
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
	(void)aMessage;
	(void)aOnResponse;
	// TODO

	return Ret::Ignored;
}

void FtpClient::onHrTimer()
{
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};

	if (requestRepeat.stateCommon.iAttempt >= knMaxAttempts) {
		ESP_LOGW(Mav::kDebugTag, "%s:%s: state \"%s\": exceeded the number of attempts", debugPreamble(), __func__,
			requestRepeat.currentStateAsString());

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
	}

	startOnce(kRequestResendTimeout);
}

void FtpClient::onFileBufferingFinished(std::shared_ptr<::Bft::File> aBftFile)
{
	std::lock_guard<std::mutex> lock{requestRepeat.mutex};

	switch (requestRepeat.state) {
		case RequestRepeat::StateIdle:
			requestRepeat.stateCommon = {0, aBftFile};
			sendSessionOpenRequest();  // XXX: from WQ?
			// TODO: launch HrTimer

			break;

		default:
			ESP_LOGW(Mav::kDebugTag, "%s:%s: already have an active session, ignoring", debugPreamble(), __func__);

			break;
	}

	// TODO: register an attempt
	// TODO: state
}

}  // namespace Mic
}  // namespace Mav
