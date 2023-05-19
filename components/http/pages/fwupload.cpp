//
// fwupload.cpp
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "pages/pages.h"


extern "C" esp_err_t fwUploadPageHandler(httpd_req_t *aHttpdReq)
{
	(void)aHttpdReq;

	return ESP_FAIL;
}
