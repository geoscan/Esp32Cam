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

			if (password.size() >= kPasswordMinLength && password.size() <= kPasswordMaxLength) {
				writeResp = {Mod::Fld::RequestResult::Ok};
			} else {
				writeResp = {Mod::Fld::RequestResult::Other, kInvalidPasswordLength};
			}

			break;
		}

		default:
			break;
	}

	onWriteResponse(writeResp);
}

}  // namespace Wifi
