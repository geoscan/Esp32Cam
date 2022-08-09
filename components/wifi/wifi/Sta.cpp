//
// Module.cpp
//
// Created on: Aug 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Sta.hpp"
#include <esp_wifi.h>

namespace Wifi {

static constexpr auto kModule = Utility::Mod::Module::WifiStaConnection;

Sta::Sta() : Utility::Mod::ModuleBase{kModule}
{
}

void Sta::getFieldValue(Utility::Mod::Fld::Req aReq, Utility::Mod::Fld::OnResponseCallback aOnResponse)
{

	switch (aReq.field) {
		case Utility::Mod::Fld::Field::Initialized: {
			wifi_ap_record_t wifiApRecord{};
			aOnResponse(makeResponse<kModule, Utility::Mod::Fld::Field::Initialized>(
				ESP_OK == esp_wifi_sta_get_ap_info(&wifiApRecord)));

			break;
		}
		default:
			break;
	}
}

}  // namespace Wifi
