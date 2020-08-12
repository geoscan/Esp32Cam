#include <esp_err.h>
#include <esp_http_server.h>

#include "pages.h"

static esp_err_t cameraHandler(httpd_req_t *req)
{
	return ESP_OK;
}

const httpd_uri_t cameraDemo = {
	.uri      = "/camera",
	.method   = HTTP_GET,
	.handler  = cameraHandler,
	.user_ctx = "Camera demo"
};


