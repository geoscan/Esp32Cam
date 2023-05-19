//
// fwupload.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Overriding local log level requires the following to be placed at the beginning of a file.
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_HTTP_DEBUG_LEVEL)
#include <esp_log.h>

#include "http.h"
#include "pages/pages.h"

/// \brief Test implementation, proof-of-concept. Reads the input file
/// chunk-by-chunk, and produces debug output.
static esp_err_t testPageHandler(httpd_req_t *aHttpdReq);

/// \brief The actual implementation
static esp_err_t pageHandler(httpd_req_t *aHttpdReq);

static constexpr const char *debugPreamble()
{
	return "fwupload";
}

static esp_err_t testPageHandler(httpd_req_t *aHttpdReq)
{
	(void)aHttpdReq;
	ESP_LOGD(httpDebugTag(), "%s - running test page handler", debugPreamble());
	httpd_resp_set_type(aHttpdReq, "text/html");
	httpd_resp_send(aHttpdReq, "Hello", 6);

	return ESP_FAIL;
}

static esp_err_t pageHandler(httpd_req_t *aHttpdReq)
{
	(void)aHttpdReq;

	return ESP_FAIL;
}

extern "C" esp_err_t fwUploadPageHandler(httpd_req_t *aHttpdReq)
{
	return testPageHandler(aHttpdReq);
}
