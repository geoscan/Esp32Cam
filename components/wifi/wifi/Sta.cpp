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
#include <esp_wifi.h>

GS_UTILITY_LOGD_METHOD_SET_ENABLED(Wifi::Sta, getFieldValue, 1)

namespace Wifi {

static constexpr auto kModule = Mod::Module::WifiStaConnection;

Sta::Sta(esp_netif_t **aEspNetif) : Mod::ModuleBase{kModule}, espNetif{aEspNetif}
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

}  // namespace Wifi
