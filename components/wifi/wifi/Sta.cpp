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

namespace Wifi {

constexpr std::size_t kSsidMinLength = 1;
constexpr std::size_t kSsidMaxLength = 32;
constexpr std::size_t kPasswordMinLength = 8;
constexpr std::size_t kPasswordMaxLength = 64;


static constexpr auto kModule = Mod::Module::WifiStaConnection;

Sta::Sta(esp_netif_t **aEspNetif) : Mod::ModuleBase{kModule}, espNetif{aEspNetif}, credentials{"", ""}
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
		case Mod::Fld::Field::Ip: {
			wifi_ap_record_t wifiApRecord{};

			if (ESP_OK == esp_wifi_sta_get_ap_info(&wifiApRecord)) {
				esp_netif_ip_info_t espNetifIpInfo{};

				if (*espNetif != nullptr && ESP_OK == esp_netif_get_ip_info(*espNetif, &espNetifIpInfo)) {
					aOnResponse(makeResponse<kModule, Mod::Fld::Field::Ip>(
						asio::ip::address_v4{espNetifIpInfo.ip.addr}));
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
		}

		onResponse(resp);
	} else if (writeReq.field == Mod::Fld::Field::Initialized) {  // Activate connection
		if (!credentials.isValid()) {
			Mod::Par::Result result = credentials.fetch();

			if (result != Mod::Par::Result::Ok || !credentials.isValid()) {
				resp = {Mod::Fld::RequestResult::Other, Mod::Par::resultAsStr(result)};
			}
		}

		if (resp.isOk()) {
			constexpr std::uint8_t *kIp = nullptr;
			constexpr std::uint8_t *kGateway = nullptr;
			constexpr std::uint8_t *kNetmask = nullptr;
			const esp_err_t espErr = wifiStaConnect(credentials.ssid.c_str(), credentials.password.c_str(), kIp,
				kGateway, kNetmask);

			if (espErr != ESP_OK) {
				resp = {Mod::Fld::RequestResult::Other, esp_err_to_name(espErr)};
			}
		}

		onResponse(resp);
	}
}

Mod::Par::Result Sta::Credentials::fetch()
{
	Mod::Par::Result result = Mod::Par::Result::Ok;
	auto *paramSsid = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiStaConnection,
		Mod::Fld::Field::StringIdentifier);

	if (paramSsid != nullptr) {
		result = paramSsid->fetch();

		if (result == Mod::Par::Result::Ok) {
			ssid = paramSsid->asStr();
		}
	} else {
		result = Mod::Par::Result::ConfigDoesNotExist;
	}

	if (result == Mod::Par::Result::Ok) {
		auto *paramPassword = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiStaConnection,
			Mod::Fld::Field::Password);

		if (paramPassword != nullptr) {
			result = paramPassword->fetch();

			if (result == Mod::Par::Result::Ok) {
				password = paramSsid->asStr();
			}
		} else {
			result = Mod::Par::Result::ConfigDoesNotExist;
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

}  // namespace Wifi
