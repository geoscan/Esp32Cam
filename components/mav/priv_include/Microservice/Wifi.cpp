//
// Wifi.cpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Mavlink.hpp"
#include "Globals.hpp"
#include "Wifi.hpp"
#include "module/ModuleBase.hpp"
#include "Helper/WifiConfigAp.hpp"
#include "Wifi.hpp"
#include "wifi.h"
#include <esp_wifi.h>

namespace Mav {
namespace Mic {

Microservice::Ret Wifi::process(mavlink_message_t &message, Microservice::OnResponseSignature onResponse)
{
	Ret ret = Ret::Ignored;

	if (message.compid == Globals::getCompId()) {
		switch (message.msgid) {
			case MAVLINK_MSG_ID_WIFI_CONFIG_AP:
				ret = processConfigAp(message, onResponse);

				break;

			default:
				break;
		}
	}

	return ret;
}

Microservice::Ret Wifi::processConfigAp(mavlink_message_t &mavlinkMessage, Microservice::OnResponseSignature onResponse)
{
	Ret ret = Ret::Response;
	Hlpr::WifiConfigAp mavlinkWifiConfig;
	mavlink_msg_wifi_config_ap_decode(&mavlinkMessage, &mavlinkWifiConfig);

	// TODO: check sysid and compid
	switch (mavlinkWifiConfig.mode) {
		// Update AP configuration
		case WIFI_CONFIG_AP_MODE::WIFI_CONFIG_AP_MODE_AP:
			ret = processConfigApSetAp(mavlinkMessage, onResponse, mavlinkWifiConfig);

			break;

		// Update STA configuration
		case WIFI_CONFIG_AP_MODE::WIFI_CONFIG_AP_MODE_STATION:
			ret = processConfigApSetSta(mavlinkMessage, onResponse, mavlinkWifiConfig);

			break;

		default:
			break;
	}

	return ret;
}

Microservice::Ret Wifi::processConfigApSetAp(mavlink_message_t &message, Microservice::OnResponseSignature onResponse,
	Hlpr::WifiConfigAp &mavlinkWifiConfigAp)
{
	mavlinkWifiConfigAp.response = WIFI_CONFIG_AP_RESPONSE::WIFI_CONFIG_AP_RESPONSE_UNDEFINED;
	// Update the field using module API
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier>(
		std::string{mavlinkWifiConfigAp.ssid, sizeof(mavlinkWifiConfigAp.ssid)},
		[&mavlinkWifiConfigAp](const Mod::Fld::WriteResp &aResponse)
		{
			if (aResponse.isOk()) {
				mavlinkWifiConfigAp.response = WIFI_CONFIG_AP_RESPONSE::WIFI_CONFIG_AP_RESPONSE_ACCEPTED;
			}
		});
	// Pack and send the response
	mavlinkWifiConfigAp.packInto(message);
	onResponse(message);

	return Ret::Response;
}

Microservice::Ret Wifi::processConfigApSetSta(mavlink_message_t &message, Microservice::OnResponseSignature onResponse,
	Hlpr::WifiConfigAp &mavlinkWifiConfigAp)
{
	// Ensure nullptr termination
	const std::string ssid{mavlinkWifiConfigAp.ssid, sizeof(mavlinkWifiConfigAp.ssid)};
	const std::string password{mavlinkWifiConfigAp.password, sizeof(mavlinkWifiConfigAp.password)};

	if (ssid.length() > 0) {
		// Try to connect, and, depending on the result of it, pack the response message
		esp_err_t staConnectEspErr = wifiStaConnect(ssid.c_str(), password.c_str(), nullptr, nullptr, nullptr);

		if (staConnectEspErr == ESP_OK) {
			mavlinkWifiConfigAp.response = WIFI_CONFIG_AP_RESPONSE::WIFI_CONFIG_AP_RESPONSE_ACCEPTED;
		} else {
			mavlinkWifiConfigAp.response = WIFI_CONFIG_AP_RESPONSE::WIFI_CONFIG_AP_RESPONSE_REJECTED;
		}

		mavlinkWifiConfigAp.packInto(message);
		onResponse(message);
	} else {
		esp_wifi_disconnect();
	}

	return Ret::Response;
}

}  // namespace Mic
}  // namespace Mav
