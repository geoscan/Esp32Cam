#include <esp_err.h>
#include <esp_http_server.h>
#include <cJSON.h>

#include "pages.h"
#include "Ov2640.hpp"
#include "version.hpp"

esp_err_t infoPageHandler(httpd_req_t *req)
{
	std::string stm32ver;
	auto *versions = cJSON_CreateArray();
	cJSON_AddItemToArray(versions, cJSON_CreateString(ESP32_FIRMWARE_VERSION));
	if (versionStmGet(stm32ver)) {
		cJSON_AddItemToArray(versions, cJSON_CreateString(stm32ver.c_str()));
	}

	char *json = cJSON_Print(versions);
	esp_err_t res = httpd_resp_send(req, json, strlen(json));

	free(json);
	cJSON_Delete(versions);

	return res;
}