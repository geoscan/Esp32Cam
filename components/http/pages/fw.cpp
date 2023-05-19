//
// fw.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "pages/pages.h"
#include <cstdint>

/// \returns HTML response text content. See also "CMakeLists.txt" and
/// "upload_script.html"
static const char *htmlResponse();

static std::size_t htmlResponseSize();

/// \brief For debugging. Reads the file it's been provided with chunk-by-chunk
static esp_err_t testPageHandler(httpd_req_t *);

/// \brief The actual upload handler which implements file uploading logic.
static esp_err_t pageHandler(httpd_req_t *aHttpdReq);

static inline const char *htmlResponse()
{
	extern char htmlStart[] asm("_binary_upload_script_html_start");

	return &htmlStart[0];
}

static inline std::size_t htmlResponseSize()
{
	extern char htmlStart[] asm("_binary_upload_script_html_start");
	extern char htmlEnd[] asm("_binary_upload_script_html_end");

	return htmlEnd - htmlStart;
}

static inline esp_err_t testPageHandler(httpd_req_t *aHttpdReq)
{
	httpd_resp_set_type(aHttpdReq, "text/html");
	httpd_resp_send(aHttpdReq, "Hello", 6);

	return ESP_OK;
}

static inline esp_err_t pageHandler(httpd_req_t *aHttpdReq)
{
	httpd_resp_set_type(aHttpdReq, "text/html");
	httpd_resp_send(aHttpdReq, htmlResponse(), htmlResponseSize());

	return ESP_OK;
}

extern "C" esp_err_t fwPageHandler(httpd_req_t *aHttpdReq)
{
	return pageHandler(aHttpdReq);
}
