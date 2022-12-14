//
// Module.cpp
//
// Created on: Aug 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_DEBUG_LEVEL)
#include <esp_log.h>

#include "Sta.hpp"
#include "wifi.h"
#include "utility/LogSection.hpp"
#include "module/Parameter/Parameter.hpp"
#include <esp_wifi.h>

GS_UTILITY_LOGD_METHOD_SET_ENABLED(Wifi::Sta, getFieldValue, 1)
GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Wifi::Sta, "tracing", 1);

namespace Wifi {

constexpr std::size_t kSsidMinLength = 1;
constexpr std::size_t kSsidMaxLength = 32;
constexpr std::size_t kPasswordMinLength = 8;
constexpr std::size_t kPasswordMaxLength = 64;
// STA network-level credentials
constexpr std::uint8_t *kIp = nullptr;
constexpr std::uint8_t *kGateway = nullptr;
constexpr std::uint8_t *kNetmask = nullptr;
static constexpr auto kModule = Mod::Module::WifiStaConnection;

static std::array<std::uint8_t, 4> u32AsBytes(std::uint32_t ip);
static std::string u32AsIpString(std::uint32_t ip);
static std::uint32_t ipStringToNetworkIp4(std::string ipString);

Sta::Sta(esp_netif_t **aEspNetif) : Mod::ModuleBase{kModule}, espNetif{aEspNetif}, credentials{"", "", false, 0, 0, 0}
{
}

void Sta::getFieldValue(Mod::Fld::Req aReq, Mod::Fld::OnResponseCallback aOnResponse)
{
	switch (aReq.field) {
		case Mod::Fld::Field::Initialized: {
			GS_UTILITY_LOGD_METHOD(Wifi::kDebugTag, Sta, getFieldValue, "Field::Initialized");
			wifi_ap_record_t wifiApRecord{};
			aOnResponse(makeResponse<kModule, Mod::Fld::Field::Initialized>(
				ESP_OK == esp_wifi_sta_get_ap_info(&wifiApRecord)));

			break;
		}
		case Mod::Fld::Field::StringIdentifier: {
			wifi_config_t wifiConfig;
			const esp_err_t espErr = esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);

			if (espErr == ESP_OK && isConnected()) {
				const std::size_t ssidLen = wifiConfig.sta.ssid[sizeof(wifiConfig.sta.ssid) - 1] == 0 ?
					strlen(reinterpret_cast<const char *>(wifiConfig.sta.ssid)) :
					sizeof(wifiConfig.sta.ssid);
				const std::string ssid{reinterpret_cast<const char *>(wifiConfig.sta.ssid), ssidLen};
				aOnResponse({ssid});
			}

			break;
		}
		case Mod::Fld::Field::Password: {
			wifi_config_t wifiConfig;
			const esp_err_t espErr = esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);

			if (espErr == ESP_OK && isConnected()) {
				const std::size_t passwordLen = wifiConfig.sta.password[sizeof(wifiConfig.sta.password) - 1] == 0 ?
					strlen(reinterpret_cast<const char *>(wifiConfig.sta.password)) :
					sizeof(wifiConfig.sta.password);
				const std::string password{reinterpret_cast<const char *>(wifiConfig.sta.password), passwordLen};
				aOnResponse({password});
			}

			break;
		}
		case Mod::Fld::Field::Ip: {
			wifi_ap_record_t wifiApRecord{};

			if (ESP_OK == esp_wifi_sta_get_ap_info(&wifiApRecord)) {
				esp_netif_ip_info_t espNetifIpInfo{};

				if (*espNetif != nullptr && ESP_OK == esp_netif_get_ip_info(*espNetif, &espNetifIpInfo)) {
					aOnResponse(makeResponse<kModule, Mod::Fld::Field::Ip>(espNetifIpInfo.ip.addr));
				}
			}
		}
		default:
			break;
	}
}

void Sta::setFieldValue(Mod::Fld::WriteReq writeReq, Mod::Fld::OnWriteResponseCallback onResponse)
{
	Mod::Fld::WriteResp resp = {Mod::Fld::RequestResult::Ok};

	if (writeReq.field == Mod::Fld::Field::Password) {
		if (!credentials.trySetPassword(writeReq.variant.getUnchecked<Mod::Module::WifiStaConnection,
				Mod::Fld::Field::StringIdentifier>())) {
			resp = {Mod::Fld::RequestResult::Other, "Wrong password format"};
		}

		onResponse(resp);
	} else if (writeReq.field == Mod::Fld::Field::StringIdentifier) {
		if (!credentials.trySetSsid(writeReq.variant.getUnchecked<Mod::Module::WifiStaConnection,
				Mod::Fld::Field::StringIdentifier>())) {
			resp = {Mod::Fld::RequestResult::Other, "Wrong SSID format"};
		} else {
			credentials.resetUseDhcp();
		}

		onResponse(resp);
	} else if (writeReq.field == Mod::Fld::Field::Initialized) {  // Activate connection
		bool enable = writeReq.variant.getUnchecked<Mod::Module::WifiStaConnection, Mod::Fld::Field::Initialized>();
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Sta, "tracing", "Setting \"Initialized\" field");

		if (enable) {
			credentials.autoconnect = true;

			if (!tryFetchConnect()) {
				resp = {Mod::Fld::RequestResult::Other, "Unable to fetch conn. credentials, or execute the connection"};
			}

			onResponse(resp);
		} else {  // Disconnect
			credentials.autoconnect = false;
			GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Sta, "tracing", "enable=%d, disconnecting", enable);
			esp_wifi_disconnect();
			onResponse(resp);
		}
	} else if (writeReq.field == Mod::Fld::Field::Ip) {
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "saving IP parameters storage");
		credentials.ip = writeReq.variant.getUnchecked<Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip>();
		onResponse(resp);
		Mod::Par::Parameter::commitStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip,
			u32AsIpString(credentials.ip));
	} else if (writeReq.field == Mod::Fld::Field::Netmask) {
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "saving netmask parameters storage");
		credentials.netmask = writeReq.variant.getUnchecked<Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip>();
		onResponse(resp);
		Mod::Par::Parameter::commitStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Netmask,
			u32AsIpString(credentials.netmask));
	} else if (writeReq.field == Mod::Fld::Field::Gateway) {
		credentials.gateway = writeReq.variant.getUnchecked<Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip>();
		onResponse(resp);
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "saving gateway IP parameters storage");
		Mod::Par::Parameter::commitStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Gateway,
			u32AsIpString(credentials.gateway));
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "saved gateway IP parameters storage");
	}
}

bool Sta::tryFetchConnect()
{
	bool result = true;
	GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Sta, "tracing", "attempting to connect");

	// Fetch credentials, if invalid
	if (!credentials.isValid()) {
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Sta, "tracing", "invalid credentials, fetching");
		Mod::Par::Result fetchResult = credentials.fetch();

		if (fetchResult != Mod::Par::Result::Ok || !credentials.isValid()) {
			ESP_LOGW(Wifi::kDebugTag, "Unable to fetch connection parameters");
			result = false;
		}
	}

	// Determine whether or not a network's DHCP should be used, and execute the appropriate connection protocol
	if (result) {
		if (credentials.autoconnect) {
			GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Sta, "tracing", "the credentials are valid, connecting");
			esp_err_t espErr = ESP_OK;

			if (credentials.useDhcp()) {
				ESP_LOGI(Wifi::kDebugTag, "Connecting using DHCP, SSID=%s", credentials.ssid.c_str());
				espErr = wifiStaConnect(credentials.ssid.c_str(), credentials.password.c_str(), kIp, kGateway,
					kNetmask);
			} else {
				const auto ip = credentials.ipAsBytes();
				const auto netmask = credentials.netmaskAsBytes();
				const auto gateway = credentials.gatewayAsBytes();
				GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Sta, "tracing", "Connecting w/ IP");
				ESP_LOGI(Wifi::kDebugTag, "Sta: Connecting w/ IP, SSID=%s, ip=%d.%d.%d.%d, gateway=%d.%d.%d.%d,"
					"netmask=%d.%d.%d.%d", credentials.ssid.c_str(), ip[0], ip[1], ip[2],
					ip[3], netmask[0], netmask[1], netmask[2], netmask[3], gateway[0], gateway[1], gateway[2], gateway[3]);
				espErr = wifiStaConnect(credentials.ssid.c_str(), credentials.password.c_str(), ip.data(),
					gateway.data(), netmask.data());
			}

			if (espErr == ESP_OK) {
				ESP_LOGI(Wifi::kDebugTag, "Sta: connection succeeded");
			} else {
				result = false;
				ESP_LOGW(Wifi::kDebugTag, "Sta: failed to connect to an AP, error=%d (%s)", espErr, esp_err_to_name(espErr));
			}
		} else {
			ESP_LOGI(Wifi::kDebugTag, "Sta: no autoconnect required, skipping");
		}
	}

	return result;
}

bool Sta::isConnected() const
{
	wifi_ap_record_t wifiApRecord;
	const bool connected = (esp_wifi_sta_get_ap_info(&wifiApRecord) == ESP_OK);

	return connected;
}

Mod::Par::Result Sta::Credentials::fetch()
{
	Mod::Par::Result result = Mod::Par::Result::Ok;
	{
		auto *paramSsid = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiStaConnection,
			Mod::Fld::Field::StringIdentifier);

		if (paramSsid != nullptr) {
			result = paramSsid->fetch();

			if (result == Mod::Par::Result::Ok) {
				ssid = paramSsid->asStr();
			}
		} else {  // Should not get here
			result = Mod::Par::Result::ConfigDoesNotExist;
		}
	}

	if (result == Mod::Par::Result::Ok) {
		auto *paramPassword = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiStaConnection,
			Mod::Fld::Field::Password);

		if (paramPassword != nullptr) {
			result = paramPassword->fetch();

			if (result == Mod::Par::Result::Ok) {
				password = paramPassword->asStr();
			}
		} else {  // Should not get here
			result = Mod::Par::Result::ConfigDoesNotExist;
		}
	}

	if (result == Mod::Par::Result::Ok) {
		auto *paramAutoconnect = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiStaConnection,
			Mod::Fld::Field::Initialized);

		if (paramAutoconnect != nullptr) {
			result = paramAutoconnect->fetch();

			if (result == Mod::Par::Result::Ok) {
				autoconnect = static_cast<bool>(paramAutoconnect->asI32());
			}
		} else {  // Should not get here
			result = Mod::Par::Result::ConfigDoesNotExist;
		}
	}

	if (result != Mod::Par::Result::Ok) {
		ESP_LOGW(Wifi::kDebugTag, "Sta::fetch: error=%d (%s)", static_cast<int>(result), Mod::Par::resultAsStr(result));
	}

	// Fetch IP
	{
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "fetching IP from parameters storage");
		std::string strIp;

		if (Mod::Par::Parameter::fetchStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip, strIp)
				== Mod::Par::Result::Ok) {
			ip = ipStringToNetworkIp4(strIp);
		}
	}
	// Fetch gateway
	{
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "fetching gateway IP from parameters"
			"storage");
		std::string strGateway;

		if (Mod::Par::Parameter::fetchStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Gateway, strGateway)
				== Mod::Par::Result::Ok) {
			gateway = ipStringToNetworkIp4(strGateway);
		}
	}
	// Fetch netmask
	{
		GS_UTILITY_LOGD_CLASS_ASPECT(Wifi::kDebugTag, Wifi::Sta, "tracing", "fetching netmask from parameters storage");
		std::string strNetmask;

		if (Mod::Par::Parameter::fetchStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Netmask, strNetmask)
				== Mod::Par::Result::Ok) {
			netmask = ipStringToNetworkIp4(strNetmask);
		}
	}

	return result;
}

bool Sta::Credentials::isValid() const
{
	return ssid.length() >= kSsidMinLength && ssid.length() <= kSsidMaxLength && (password.length() == 0
		|| (password.length() >= kPasswordMinLength && password.length() <= kPasswordMaxLength));
}

bool Sta::Credentials::trySetPassword(const std::string &password)
{
	bool ret = password.length() == 0 || (password.length() >= kPasswordMinLength
		&& password.length() <= kPasswordMaxLength);

	if (ret) {
		this->password = password;
	}

	return ret;
}

bool Sta::Credentials::trySetSsid(const std::string &ssid)
{
	bool ret = ssid.length() >= kSsidMinLength && ssid.length() <= kSsidMaxLength;

	if (ret) {
		this->ssid = ssid;
	}

	return ret;
}

static std::array<std::uint8_t, 4> u32AsBytes(std::uint32_t u32)
{
	u32 = htonl(u32);
	const std::uint8_t *bytes = reinterpret_cast<const std::uint8_t *>(&u32);

	return {{bytes[0], bytes[1], bytes[2], bytes[3]}};
}

static std::string u32AsIpString(std::uint32_t ipNetwork)
{
	return asio::ip::address_v4{ipNetwork}.to_string();
}

static std::uint32_t ipStringToNetworkIp4(std::string ipString)
{
	asio::error_code err;
	const auto addr = asio::ip::make_address_v4(ipString, err);
	std::uint32_t res = 0;

	if (!err) {
		res = addr.to_uint();
	}

	return res;
}

std::array<uint8_t, 4> Sta::Credentials::ipAsBytes() const
{
	return u32AsBytes(ip);
}

std::array<uint8_t, 4> Sta::Credentials::netmaskAsBytes() const
{
	return u32AsBytes(netmask);
}

std::array<uint8_t, 4> Sta::Credentials::gatewayAsBytes() const
{
	return u32AsBytes(gateway);
}

void Sta::Credentials::resetUseDhcp()
{
	ip = 0;
	Mod::Par::Parameter::commitStringByMf(Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip, "");
}

}  // namespace Wifi
