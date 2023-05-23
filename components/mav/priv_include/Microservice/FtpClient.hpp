//
// FtpClient.hpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_MAV_PRIV_INCLUDE_MICROSERVICE_FTPCLIENT_HPP_
#define COMPONENTS_MAV_PRIV_INCLUDE_MICROSERVICE_FTPCLIENT_HPP_

#include "DelayedSend.hpp"
#include "Microservice.hpp"
#include "buffered_file_transfer/Sub.hpp"

namespace Mav {
namespace Mic {

/// \brief Implements client side of MAVLink File Transfer Protocol for
/// flashing file into AP's FS.
class FtpClient final : public Microservice, public DelayedSend {
private:
	struct SubscriptionPackage {
		Bft::OnFileBufferingFinished onFileBufferingFinished;
	};

public:
	FtpClient();
	~FtpClient();
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;

private:
	void onFileBufferingFinished(std::shared_ptr<::Bft::File>);

private:
	SubscriptionPackage subscriptionPackage;
};

}  // namespace Mic
}  // namespace Mav

#endif // FTPCLIENT_HPP_
