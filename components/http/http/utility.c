//
// utility.c
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_http_server.h>
#include <stddef.h>

void httpdReqParameterValue(httpd_req_t *aHttpdReq, const char *aParameterKey, char *aValueBuffer, size_t aValueBufferSize)
{
	static const kMinBufferSize = 2;
	esp_err_t ret = ESP_OK;

	if (aValueBufferSize < kMinBufferSize) {
		return ESP_ERR_INVALID_ARG;
	}

	aValueBuffer[aValueBufferSize - 1] = '\0';  // Ensure null-termination

	// Extract query from URL
	ret = httpd_req_get_url_query_str(aHttpdReq, aValueBuffer, aValueBufferSize - 1);

	if (ESP_OK != ret) {
		return ret;
	}

	// Extract value from the query. It must be safe to use the same buffer.
	// `httpd_query_key_value` uses `strlcpy`, therefore, no boundary checks
	// are required, and there is no possibility of overwriting the resulting
	// string.
	ret = httpd_query_key_value(aValueBuffer, aParameterKey, aValueBuffer, aValueBufferSize - 1);

	return ret;
}
