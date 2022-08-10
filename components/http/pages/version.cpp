#include "pages.h"
#include "Ov2640.hpp"
#include "version.hpp"
#include "module/ModuleBase.hpp"
#include <cstring>
#include <esp_err.h>
#include <esp_http_server.h>
#include <cJSON.h>

esp_err_t infoPageHandler(httpd_req_t *req)
{
	auto *versions = cJSON_CreateArray();
	cJSON_AddItemToArray(versions, cJSON_CreateString(ESP32_FIRMWARE_VERSION));  // Add ESP version to the JSON object

	// Form the autopilot's version string
	{
		std::uint32_t revMajor = 0;
		std::uint32_t revMinor = 0;
		std::uint32_t revPatch = 0;
		std::uint32_t hashCommit = 0;

		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Autopilot,
			Mod::Fld::Field::VersionSoftwareMajor>([&revMajor](unsigned a){revMajor = a;});
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Autopilot,
			Mod::Fld::Field::VersionSoftwareMinor>([&revMinor](unsigned a){revMinor = a;});
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Autopilot,
			Mod::Fld::Field::VersionSoftwarePatch>([&revPatch](unsigned a){revPatch = a;});
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Autopilot,
			Mod::Fld::Field::VersionCommitHash>([&hashCommit](unsigned a){hashCommit = a;});

		const auto versionStringLength = snprintf(nullptr, 0, "%d.%d.%d-%x", revMajor, revMinor, revPatch, hashCommit);
		char versionString[versionStringLength + 1] = {0};
		snprintf(versionString, versionStringLength + 1, "%d.%d.%d-%x", revMajor, revMinor, revPatch, hashCommit);
		cJSON_AddItemToArray(versions, cJSON_CreateString(versionString));  // Add AP's version to the JSON object
	}

	char *json = cJSON_Print(versions);  // Compose JSON string
	esp_err_t res = httpd_resp_send(req, json, strlen(json));  // Send HTTP response

	free(json);
	cJSON_Delete(versions);

	return res;
}
