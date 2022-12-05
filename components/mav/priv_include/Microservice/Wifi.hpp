//
// WiFi.hpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MAV_PRIV_INCLUDE_MICROSERVICE_WIFI_HPP_)
#define MAV_PRIV_INCLUDE_MICROSERVICE_WIFI_HPP_

#include "Microservice.hpp"
#include "Mavlink.hpp"

namespace Mav {
namespace Hlpr {

class WifiConfigAp;
class MavlinkCommandLong;

}  // namespace Hlpr

namespace Mic {

/// \brief Implements processing of Wi-Fi-related MAVLink messages
class Wifi : public Microservice {
public:
	Ret process(mavlink_message_t &aMessage, OnResponseSignature aOnResponse) override;
private:
	/// \brief Process WIFI_CONFIG_AP message
	/// http://mavlink.io/en/messages/common.html#WIFI_CONFIG_AP
	Ret processConfigAp(mavlink_message_t &aMessage, OnResponseSignature aOnResponse);
	/// \brief Process the request to change SSID and password;
	Ret processConfigApSetAp(mavlink_message_t &message, OnResponseSignature onResponse, Hlpr::WifiConfigAp &);
	/// \brief Process a request to connect to / disconnect from a remote AP
	Ret processConfigApSetSta(mavlink_message_t &message, OnResponseSignature onResponse, Hlpr::WifiConfigAp &);
	/// \brief
	Ret processCmdRequestMessage(mavlink_message_t &message, OnResponseSignature onResponse,
		const Hlpr::MavlinkCommandLong &mavlinkCommandLong);
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_WIFI_HPP_
