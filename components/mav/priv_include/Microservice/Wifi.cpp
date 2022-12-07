//
// Wifi.cpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>
#include "Mavlink.hpp"
#include "Globals.hpp"
#include "Wifi.hpp"
#include "module/ModuleBase.hpp"
#include "Helper/WifiConfigAp.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "Helper/MavlinkCommandAck.hpp"
#include "Wifi.hpp"
#include "wifi.h"
#include "mav/mav.hpp"
#include "utility/LogSection.hpp"
#include <esp_wifi.h>

GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Mav::Mic::Wifi, "AP settings", 1);
GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Mav::Mic::Wifi, "tracing", 1);

namespace Mav {
namespace Mic {

Microservice::Ret Wifi::process(mavlink_message_t &message, Microservice::OnResponseSignature onResponse)
{
	Ret ret = Ret::Ignored;
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "got message");

	switch (message.msgid) {
		case MAVLINK_MSG_ID_WIFI_CONFIG_AP:
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Mav::Mic::Wifi, "tracing", "got WIFI_CONFIG_AP");
			ret = processConfigAp(message, onResponse);

			break;

		case MAVLINK_MSG_ID_COMMAND_INT:
		// falls through
		case MAVLINK_MSG_ID_COMMAND_LONG: {
			const auto mavlinkCommandLong = Hlpr::MavlinkCommandLong::makeFrom(message);
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Mav::Mic::Wifi, "tracing", "got COMMAND_LONG, sysid=%d,"
				"compid=%d, command=%d", mavlinkCommandLong.target_system, mavlinkCommandLong.target_component,
				mavlinkCommandLong.command);

			if (mavlinkCommandLong.target_system == Globals::getSysId()
					&& mavlinkCommandLong.target_component == Globals::getCompId()) {
				switch (mavlinkCommandLong.command) {
					case MAV_CMD_REQUEST_MESSAGE:
						ret = processCmdRequestMessage(message, onResponse, mavlinkCommandLong);

						break;

					default:
						break;
				}
			}

			break;
		}

		default:
			break;
	}

	return ret;
}

Microservice::Ret Wifi::processConfigAp(mavlink_message_t &mavlinkMessage, Microservice::OnResponseSignature onResponse)
{
	Ret ret = Ret::Response;
	Hlpr::WifiConfigAp mavlinkWifiConfig;
	mavlink_msg_wifi_config_ap_decode(&mavlinkMessage, &mavlinkWifiConfig);
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigAp, mode=%d", mavlinkWifiConfig.mode);
	// [0x00, 0xFF] is a special sequence denoting that a field is not used. Certain combinations of field values
	// map to certain requests
	const bool hasSsid = mavlinkWifiConfig.ssid[0] != 0x00 && mavlinkWifiConfig.ssid[1] != 0xFF;
	const bool hasPassword = mavlinkWifiConfig.password[0] != 0x00 && mavlinkWifiConfig.password[1] != 0xFF;

	if (mavlinkWifiConfig.mode == WIFI_CONFIG_AP_MODE_UNDEFINED) {
		// There are 2 implementations: legacy, and modern. The legacy is a
		// liability that has to be carried due to limitations of some
		// application-level libraries (such as PyMAVLink). "Undefined" mode,
		// corresponding to 0, has been decided to be used as a marker that the
		// legacy implementation must be used. It has technical reasons: due to
		// how the decoder works, 0 ("undefined mode") is produced as as result
		// of decoding MAVLink 1 variant of the message.
		if (hasSsid && hasPassword) {
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigAp legacy - connect, mode=%d",
				mavlinkWifiConfig.mode);
			ret = processConfigApConnect(mavlinkMessage, onResponse, mavlinkWifiConfig);
		} else if (!hasSsid && !hasPassword) {
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigAp legacy - disconnect,"
				"mode=%d", mavlinkWifiConfig.mode);
			ret = processConfigApDisconnect(mavlinkMessage, onResponse, mavlinkWifiConfig);
		} else if (hasSsid && !hasPassword) {
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigAp legacy - set SSId, mode=%d",
				mavlinkWifiConfig.mode);
			ret = processConfigApSetSsid(mavlinkMessage, onResponse, mavlinkWifiConfig);
		} else if (!hasSsid && hasPassword) {
			GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigAp legacy - set password,"
				"mode=%d", mavlinkWifiConfig.mode);
			ret = processConfigApSetSta(mavlinkMessage, onResponse, mavlinkWifiConfig);
		}
	} else {
		if (mavlinkWifiConfig.mode == WIFI_CONFIG_AP_MODE_AP) {
			processConfigApSetAp(mavlinkMessage, onResponse, mavlinkWifiConfig);
		} else if (mavlinkWifiConfig.mode == WIFI_CONFIG_AP_MODE_STATION) {
			processConfigApSetSta(mavlinkMessage, onResponse, mavlinkWifiConfig);
		}
	}

	return ret;
}

Microservice::Ret Wifi::processConfigApConnect(mavlink_message_t &message,
	Microservice::OnResponseSignature onResponse, Hlpr::WifiConfigAp &wifiConfigAp)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigApConnect");
	// Ensure null-termination when working w/ c-strings.
	const std::string ssid{wifiConfigAp.ssid, sizeof(wifiConfigAp.ssid)};
	const std::string password{wifiConfigAp.password, sizeof(wifiConfigAp.password)};

	constexpr std::uint8_t *kIp = nullptr;
	constexpr std::uint8_t *kGateway = nullptr;
	constexpr std::uint8_t *kNetmask = nullptr;
	const esp_err_t connectResult = wifiStaConnect(ssid.c_str(), password.c_str(), kIp, kGateway, kNetmask);
	constexpr std::array<std::uint8_t, 2> kConnectError{{0x00, 0x01}};  // See the doc.

	if (connectResult != ESP_OK) {
		wifiConfigAp.ssidFillZero();
		std::copy_n(kConnectError.begin(), kConnectError.size(), wifiConfigAp.ssid);
	}

	// Provide the response
	wifiConfigAp.packInto(message);
	onResponse(message);

	return Microservice::Ret::Response;
}

Microservice::Ret Wifi::processConfigApDisconnect(mavlink_message_t &mavlinkMessage,
	Microservice::OnResponseSignature onResponse, Hlpr::WifiConfigAp &wifiConfigAp)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigApDisconnect");
	esp_wifi_disconnect();  // No reason to handle its result, as the operation is idempotent
	// Provide the response
	wifiConfigAp.packInto(mavlinkMessage);
	onResponse(mavlinkMessage);

	return Microservice::Ret::Response;
}

Microservice::Ret Wifi::processConfigApSetSsid(mavlink_message_t &mavlinkMessage,
	Microservice::OnResponseSignature onResponse, Hlpr::WifiConfigAp &wifiConfigAp)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigApSetSsid");
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "setting AP SSID, ssid=%s", wifiConfigAp.ssid);
	const std::string ssid{wifiConfigAp.ssid, sizeof(wifiConfigAp.ssid)};
	bool success = false;
	// Set module field
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier>(ssid,
		[&wifiConfigAp, &success](const Mod::Fld::WriteResp &writeResponse)
		{
			success = writeResponse.isOk();
		});

	// Initialize error code, if needed
	if (!success) {
		ESP_LOGW(Mav::kDebugTag, "Wifi::set SSID: failed");
		constexpr std::array<char, 2> kSetSsidError = {{0x00, 0x03}};
		wifiConfigAp.ssidFillZero();
		std::copy_n(kSetSsidError.begin(), kSetSsidError.size(), wifiConfigAp.ssid);
	} else {
		ESP_LOGI(Mav::kDebugTag, "Wifi: set SSID: succeeded");
	}

	// Provide the response
	wifiConfigAp.packInto(mavlinkMessage);
	onResponse(mavlinkMessage);

	return Microservice::Ret::Response;
}

Microservice::Ret Wifi::processConfigApSetPassword(mavlink_message_t &mavlinkMessage,
	Microservice::OnResponseSignature onResponse, Hlpr::WifiConfigAp &wifiConfigAp)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "processConfigApSetPassword");
	// The password setting functionality is yet to be implemented. Send a stub
	constexpr std::array<char, 2> kSetPasswordError = {{0x00, 0x04}};
	wifiConfigAp.ssidFillZero();
	std::copy_n(kSetPasswordError.begin(), kSetPasswordError.size(), wifiConfigAp.ssid);
	// Provide the response
	wifiConfigAp.packInto(mavlinkMessage);
	onResponse(mavlinkMessage);

	return Microservice::Ret::Response;
}

Microservice::Ret Wifi::processConfigApSetAp(mavlink_message_t &message, Microservice::OnResponseSignature onResponse,
	Hlpr::WifiConfigAp &mavlinkWifiConfigAp)
{
	GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Wifi, "tracing", "setting AP");
	mavlinkWifiConfigAp.response = WIFI_CONFIG_AP_RESPONSE::WIFI_CONFIG_AP_RESPONSE_UNDEFINED;
	// Update the field using module API
	Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier>(
		std::string{mavlinkWifiConfigAp.ssid, sizeof(mavlinkWifiConfigAp.ssid)},
		[&mavlinkWifiConfigAp](const Mod::Fld::WriteResp &aResponse)
		{
			if (aResponse.isOk()) {
				mavlinkWifiConfigAp.response = WIFI_CONFIG_AP_RESPONSE::WIFI_CONFIG_AP_RESPONSE_ACCEPTED;
				ESP_LOGI(Mav::kDebugTag, "Wifi, successfully set SSID");
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

Microservice::Ret Wifi::processCmdRequestMessage(mavlink_message_t &message,
	Microservice::OnResponseSignature onResponse, const Hlpr::MavlinkCommandLong &mavlinkCommandLong)
{
	auto ret = Ret::Ignored;
	const int requestedMsgid = static_cast<int>(mavlinkCommandLong.param1);

	if (requestedMsgid == MAVLINK_MSG_ID_WIFI_CONFIG_AP) {
		GS_UTILITY_LOGD_CLASS_ASPECT(Mav::kDebugTag, Mav::Mic::Wifi, "tracing", "got request for WIFI_CONFIG_AP");
		ret = Ret::Response;
		// `param2` is used as mode identifier.
		const int mode = static_cast<int>(mavlinkCommandLong.param2);
		Hlpr::WifiConfigAp wifiConfigAp{mavlink_wifi_config_ap_t{
			{""},  // SSID
			{""},  // Password
			0,  // Mode
			0,  // Response
		}};

		// Attempting to acquire the SSID of the AP the station is connected to
		if (mode == WIFI_CONFIG_AP_MODE::WIFI_CONFIG_AP_MODE_STATION) {
			wifiConfigAp.mode = WIFI_CONFIG_AP_MODE::WIFI_CONFIG_AP_MODE_DISABLED;
			Mod::ModuleBase::moduleFieldReadIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::StringIdentifier>(
				[&wifiConfigAp](const std::string &identifier) mutable
				{
					std::copy(identifier.begin(), identifier.end(), wifiConfigAp.ssid);  // The identifier is supposed to already be of the appropriate length and format, so no checks are made
				});
		} else if (mode == WIFI_CONFIG_AP_MODE::WIFI_CONFIG_AP_MODE_AP) {
			// TODO: acquire the curret Wi-Fi AP SSID
		}

		// Compose ACK
		{
			auto ack = Hlpr::MavlinkCommandAck::makeFrom(message, mavlinkCommandLong.command, MAV_RESULT_ACCEPTED);
			ack.packInto(message);
			onResponse(message);
		}
		// Send the message itself
		wifiConfigAp.packInto(message);
		onResponse(message);
	}

	return ret;
}

}  // namespace Mic
}  // namespace Mav
