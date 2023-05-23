//
// FtpClient.cpp
//
// Created on: May 23, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "FtpClient.hpp"

namespace Mav {
namespace Mic {

Microservice::Ret FtpClient::process(mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	(void)aMessage;
	(void)aOnResponse;

	return Ret::Ignored;
}

}  // namespace Mic
}  // namespace Mav
