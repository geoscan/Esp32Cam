//
// Ap.cpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_log.h>
#include "Ap.hpp"
#include "module/module.hpp"

namespace Wifi {

static constexpr std::size_t kSsidMinLength = 1;
static constexpr std::size_t kSsidMaxLength = 32;
static constexpr const char *kInvalidSsidLengthMsg = "Invalid SSID length";

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

		default:
			break;
	}

	onWriteResponse(writeResp);
}

}  // namespace Wifi
