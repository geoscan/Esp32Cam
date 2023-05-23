//
// FtpClient.cpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "FtpClient.hpp"

namespace Mav {
namespace Mic {

FtpClient::FtpClient():
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

	return Ret::Ignored;
}

void FtpClient::onFileBufferingFinished(std::shared_ptr<::Bft::File> aBftFile)
{
	(void)aBftFile;
	// TODO:
}

}  // namespace Mic
}  // namespace Mav
