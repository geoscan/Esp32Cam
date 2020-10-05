#include <esp_err.h>
#include <esp_http_server.h>

#include "pages.h"
#include "Ov2640.hpp"
#include "version.hpp"

esp_err_t infoPageHandler(httpd_req_t *req)
{
//    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
//    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

//    const char* resp_str = (const char*) req->user_ctx;
	esp_err_t res = httpd_resp_send(req, ESP32_FIRMWARE_VERSION, HTTPD_RESP_USE_STRLEN);

	return res;
}