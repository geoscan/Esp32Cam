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
	///
	/// \details Performs handling of `WIFI_CONFIG_AP` message. The
	/// implementation is inconsistent with the standard, so please refer to
	/// the documentation (TODO: doc. link)
	Ret processConfigAp(mavlink_message_t &mavlinkMessage, OnResponseSignature aOnResponse);
	/// \brief Process connection to an AP
	Ret processConfigApConnect(mavlink_message_t &mavlinkMessage, OnResponseSignature onResponse, Hlpr::WifiConfigAp &);
	/// \brief Process disconnection from an AP
	Ret processConfigApDisconnect(mavlink_message_t &mavlinkMessage, OnResponseSignature onResponse,
		Hlpr::WifiConfigAp &);
	/// \brief Process setting SSID
	Ret processConfigApSetSsid(mavlink_message_t &mavlinkMessage, OnResponseSignature onResponse, Hlpr::WifiConfigAp &);
	/// \brief Process setting a password
	Ret processConfigApSetPassword(mavlink_message_t &mavlinkMessage, OnResponseSignature onResponse,
		Hlpr::WifiConfigAp &);
	/// \brief Process the request to change SSID and password;
	Ret processConfigApSetAp(mavlink_message_t &message, OnResponseSignature onResponse, Hlpr::WifiConfigAp &);
	/// \brief Process a request to connect to / disconnect from a remote AP
	Ret processConfigApSetSta(mavlink_message_t &message, OnResponseSignature onResponse, Hlpr::WifiConfigAp &);
	/// \brief Handles message request, as defined by the standard.
	Ret processCmdRequestMessage(mavlink_message_t &message, OnResponseSignature onResponse,
		const Hlpr::MavlinkCommandLong &mavlinkCommandLong);
	/// \brief Delegate. Prepares and sends WIFI_CONFIG_AP containing info
	/// regarding current STA configuration. \sa `processCmdRequestMessage`
	Ret processCmdRequestMessageStaStatus(mavlink_message_t &message, OnResponseSignature onResponse,
		const Hlpr::MavlinkCommandLong &mavlinkCommandLong);
};

}  // namespace Mic
}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MICROSERVICE_WIFI_HPP_
