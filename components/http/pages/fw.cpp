//
// fw.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "pages/pages.h"
#include <cstdint>

static const char *htmlResponse();
static std::size_t htmlResponseSize();

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

extern "C" esp_err_t fwPageHandler(httpd_req_t *aHttpdReq)
{
	(void)aHttpdReq;
	httpd_resp_set_type(aHttpdReq, "text/html");
//	httpd_resp_send(aHttpdReq, "Hello", 6);
	httpd_resp_send(aHttpdReq, htmlResponse(), htmlResponseSize());

	return ESP_OK;
}
