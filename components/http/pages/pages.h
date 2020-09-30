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
//#include "protocol_examples_common.h"

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t cameraDemoHandler(httpd_req_t *);
esp_err_t indexPageHandler(httpd_req_t *);
esp_err_t infoPageHandler(httpd_req_t *);

#ifdef __cplusplus
}
#endif

const httpd_uri_t cameraDemo = {
	.uri      = "/camera",
//	.method   = HTTP_POST,
	.method   = HTTP_GET,
	.handler  = cameraDemoHandler,
	.user_ctx = 0
};

const httpd_uri_t infoPage = {
	.uri = "/info",
	.method = HTTP_GET,
	.handler = infoPageHandler,
	.user_ctx = 0,
};

const httpd_uri_t indexPage = {
	.uri      = "/hello",
//	.method   = HTTP_POST,
	.method   = HTTP_GET,
	.handler  = indexPageHandler,
	.user_ctx = (void*)"Hello World!"
};


#endif // COMPONENTS_HTTP_CAMERADEMO_CAMERADEMO_H
