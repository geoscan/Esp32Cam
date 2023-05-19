//
// utility.h
//
// Created on: May 19, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_HTTP_HTTP_UTILITY_H_
#define COMPONENTS_HTTP_HTTP_UTILITY_H_

#include <esp_http_server.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/// \brief Extracts value pairs from URI corresponding to keys
esp_err_t httpdReqParameterValue(httpd_req_t aHttpdReq, const char *aParameterKey, char *aValueBuffer,
	size_t aValueBufferSize);

#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // COMPONENTS_HTTP_HTTP_UTILITY_H_
