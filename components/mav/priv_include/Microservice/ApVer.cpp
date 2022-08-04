//
// ApVer.cpp
//
// Created on: Jun 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include "ApVer.hpp"
#include "Mavlink.hpp"
#include "Globals.hpp"
#include "Helper/MavlinkCommandLong.hpp"
#include "mav/mav.hpp"

namespace Mav {
namespace Mic {

ApVer::ApVer() : ModuleBase{Utility::Mod::Module::Autopilot}, version{}, updated{}
{
}

/// \brief Implements the "heartbeat - version request - version save" communication sequence
///
Microservice::Ret ApVer::process(mavlink_message_t &aMessage, Microservice::OnResponseSignature aOnResponse)
{
	auto ret = Microservice::Ret::Ignored;

#if CONFIG_MAV_DEBUG_LEVEL >= 4
	if (MAVLINK_MSG_ID_HEARTBEAT == aMessage.msgid) {
		ESP_LOGD(Mav::kDebugTag, "ApVer::process() got HEARTBEAT");
	} else if (MAVLINK_MSG_ID_AUTOPILOT_VERSION == aMessage.msgid) {
		ESP_LOGD(Mav::kDebugTag, "ApVer::process() got AUTOPILOT_VERSION");
	}
#endif

	if (Globals::getCompidAutopilot() == aMessage.compid) {

		switch (aMessage.msgid) {
			case MAVLINK_MSG_ID_HEARTBEAT: {
				ESP_LOGD(Mav::kDebugTag, "ApVer::process() got heartbeat from compid %d", aMessage.compid);

				if (!updated.version) {
					ESP_LOGD(Mav::kDebugTag, "ApVer::process() requesting message version from compid %d",
						aMessage.compid);

					Hlpr::MavlinkCommandLong().initTargetFromRequest(aMessage)
						.initRequestMessage(MAVLINK_MSG_ID_AUTOPILOT_VERSION).packInto(aMessage);
					aOnResponse(aMessage);
					ret = Microservice::Ret::Response;
				} else {
					ret = Microservice::Ret::Ignored;
				}

				break;
			}
			case MAVLINK_MSG_ID_AUTOPILOT_VERSION: {
				ESP_LOGI(Mav::kDebugTag, "ApVer::process() got AUTOPILOT_VERSION message from compid %d",
					aMessage.compid);

				updated.version = true;
				mavlink_autopilot_version_t mavlinkAutopilotVersion{};
				mavlink_msg_autopilot_version_decode(&aMessage, &mavlinkAutopilotVersion);

				// The following code relies on knowing how revision and hash fields are packed inside the message
				version.swCommit = reinterpret_cast<std::uint32_t &>(mavlinkAutopilotVersion.flight_custom_version[0]);
				version.swRev = reinterpret_cast<std::uint32_t &>(mavlinkAutopilotVersion.flight_custom_version[4]);
				version.swMajor = (mavlinkAutopilotVersion.flight_sw_version & 0xFF00) >> 8;
				version.swMinor = mavlinkAutopilotVersion.flight_sw_version & 0xFF;

				ret = Microservice::Ret::Ignored;

				break;
			}

			default:
				break;
		}

	}

	return ret;
}

void ApVer::getFieldValue(Utility::Mod::Fld::Req aReq, Utility::Mod::Fld::OnResponseCallback aOnResponse)
{
	if (updated.version) {
		switch (aReq.field) {
			case Utility::Mod::Fld::Field::VersionCommitHash:
				aOnResponse(makeResponse<Utility::Mod::Module::Autopilot,
					Utility::Mod::Fld::Field::VersionCommitHash>(version.swCommit));

				break;

			case Utility::Mod::Fld::Field::VersionSoftwareMajor:
				aOnResponse(makeResponse<Utility::Mod::Module::Autopilot,
					Utility::Mod::Fld::Field::VersionSoftwareMajor>(version.swMajor));

				break;

			case Utility::Mod::Fld::Field::VersionSoftwareMinor:
				aOnResponse(makeResponse<Utility::Mod::Module::Autopilot,
					Utility::Mod::Fld::Field::VersionSoftwareMinor>(version.swMinor));

				break;

			case Utility::Mod::Fld::Field::VersionSoftwarePatch:
				aOnResponse(makeResponse<Utility::Mod::Module::Autopilot,
					Utility::Mod::Fld::Field::VersionSoftwarePatch>(version.swRev));

				break;

			default:
				break;
		}
	}
}

}  // namespace Mic
}  // namespace Mav
