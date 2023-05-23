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

namespace Mav {
namespace Mic {


class FtpClient : public Microservice, public DelayedSend {
public:
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;
};

}  // namespace Mic
}  // namespace Mav

#endif // FTPCLIENT_HPP_
