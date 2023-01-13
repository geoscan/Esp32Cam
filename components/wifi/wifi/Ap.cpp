//
// Ap.cpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Ap.hpp"
#include "module/module.hpp"
#include <esp_log.h>
#include <esp_wifi.h>

namespace Wifi {

static constexpr std::size_t kSsidMinLength = 1;
static constexpr std::size_t kSsidMaxLength = 32;
static constexpr std::size_t kPasswordMinLength = 8;
static constexpr std::size_t kPasswordMaxLength = 64;
static constexpr const char *kInvalidSsidLengthMsg = "Invalid SSID length";
static constexpr const char *kInvalidPasswordLength = "Invalid password length, must be between 8 and 64";

Ap::Ap() : ModuleBase{Mod::Module::WifiAp}
{
}

void Ap::setFieldValue(Mod::Fld::WriteReq request, Mod::Fld::OnWriteResponseCallback onWriteResponse)
{
	Mod::Fld::WriteResp writeResp{Mod::Fld::RequestResult::Ok};

	switch (request.field) {
		case Mod::Fld::Field::StringIdentifier: {  // SSID
			const std::string &ssid = request.variant
				.getUnchecked<Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier>();

			if (!(ssid.length() >= kSsidMinLength && ssid.length() <= kSsidMaxLength)) {
				ESP_LOGW(Mod::kDebugTag, "set StringIdentifier: %s", kInvalidSsidLengthMsg);
				writeResp = {Mod::Fld::RequestResult::Other, kInvalidSsidLengthMsg};
			}

			break;
		}
		case Mod::Fld::Field::Password: {
			const std::string &password = request.variant.getUnchecked<Mod::Module::WifiAp, Mod::Fld::Field::Password>();

			if (!(password.size() >= kPasswordMinLength && password.size() <= kPasswordMaxLength)) {
				ESP_LOGW(Mod::kDebugTag, "set Password, length=%d. %s", password.length(), kInvalidSsidLengthMsg);
				writeResp = {Mod::Fld::RequestResult::Other, kInvalidPasswordLength};
			}

			break;
		}

		default:
			break;
	}

	onWriteResponse(writeResp);
}

void Ap::getFieldValue(Mod::Fld::Req req, Mod::Fld::OnResponseCallback onResponse)
{
	if (req.field == Mod::Fld::Field::StringIdentifier) {
		wifi_config_t wifiConfig;
		const esp_err_t espErr = esp_wifi_get_config(WIFI_IF_AP, &wifiConfig);

		if (espErr == ESP_OK) {
			const std::size_t ssidLen = wifiConfig.ap.ssid[sizeof(wifiConfig.ap.ssid) - 1] == 0 ?
				strlen(reinterpret_cast<const char *>(wifiConfig.ap.ssid)) :
				sizeof(wifiConfig.ap.ssid);
			const std::string ssid{reinterpret_cast<const char *>(wifiConfig.ap.ssid), ssidLen};
			onResponse({ssid});
		}
	} else if (req.field == Mod::Fld::Field::Password) {
		wifi_config_t wifiConfig;
		const esp_err_t espErr = esp_wifi_get_config(WIFI_IF_AP, &wifiConfig);

		if (espErr == ESP_OK) {
			const std::size_t passwordLen = wifiConfig.ap.password[sizeof(wifiConfig.ap.password) - 1] == 0 ?
				strlen(reinterpret_cast<const char *>(wifiConfig.ap.password)) :
				sizeof(wifiConfig.ap.password);
			const std::string password{reinterpret_cast<const char *>(wifiConfig.ap.password), passwordLen};
			onResponse({password});
		}
	} else if (req.field == Mod::Fld::Field::Mac) {
		esp_efuse_mac_get_default(mac.data());
		onResponse({&mac});
	}
}

}  // namespace Wifi
