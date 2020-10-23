#include <esp_err.h>
#include <esp_http_server.h>

#include "pages.h"
#include "Ov2640.hpp"
#include "version.hpp"

esp_err_t infoPageHandler(httpd_req_t *req)
{
	std::string response;

	response.append("[");
	response.append("\"");
	response.append(ESP32_FIRMWARE_VERSION);
	response.append("\"");

	std::string stm32ver;
	if (versionStmGet(stm32ver)) {
		response.append(", ");
		response.append("\"");
		response.append(stm32ver);
		response.append("\"");
	}
	response.append("]");

	esp_err_t res = httpd_resp_send(req, response.data(), response.size());

	return res;
}