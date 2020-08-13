#ifndef COMPONENTS_HTTP_CAMERADEMO_PAGES_H
#define COMPONENTS_HTTP_CAMERADEMO_PAGES_H

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "sdkconfig.h"
#include "protocol_examples_common.h"

#include <esp_http_server.h>


#if !defined __cplusplus
# define __ext extern
#else
# define __ext
#endif

__ext esp_err_t cameraDemoHandler(httpd_req_t *);

#undef __ext

const httpd_uri_t cameraDemo = {
	.uri      = "/camera",
	.method   = HTTP_GET,
	.handler  = cameraDemoHandler,
	.user_ctx = 0
};


#endif // COMPONENTS_HTTP_CAMERADEMO_CAMERADEMO_H
