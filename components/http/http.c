//
// http.c
//
// Created on: ?
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_HTTP_DEBUG_LEVEL)
#include <esp_log.h>
#include "pages/pages.h"
#include <sdkconfig.h>

const char *kHttpDebugTag = "[http]";

static const httpd_uri_t pages[] = {
	{
		.uri      = "/camera/demo",
		.method   = HTTP_GET,
		.handler  = cameraDemoHandler,
		.user_ctx = 0
	},
	{
		.uri = "/info",
		.method = HTTP_GET,
		.handler = infoPageHandler,
		.user_ctx = 0,
	},
	{
		.uri      = "/hello",
		.method   = HTTP_GET,
		.handler  = indexPageHandler,
		.user_ctx = (void*)"Hello World!"
	},
	{
		.uri      = "/control",
		.method   = HTTP_GET,
		.handler  = controlHandler,
		.user_ctx = 0
	}
};

static const size_t kNpages = sizeof(pages) / sizeof(httpd_uri_t);

static httpd_handle_t startWebserver(void)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.stack_size += 1024;

	// Start the httpd server
	if (httpd_start(&server, &config) == ESP_OK) {
		for (size_t i = 0; i < kNpages; ++i) {
			httpd_register_uri_handler(server, &pages[i]);
		}
		return server;
	}

	return NULL;
}

static void stopWebserver(httpd_handle_t server)
{
	httpd_stop(server);
}

static void disconnectHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	httpd_handle_t* server = (httpd_handle_t*) arg;
	if (*server) {
		stopWebserver(*server);
		*server = NULL;
	}
}

static void connectHandler(void* arg, esp_event_base_t event_base,
							int32_t event_id, void* event_data)
{
	httpd_handle_t* server = (httpd_handle_t*) arg;
	if (*server == NULL) {
		*server = startWebserver();
	}
}

void httpStart(void)
{
	static httpd_handle_t server = NULL;
	esp_log_level_set(kHttpDebugTag, (esp_log_level_t)CONFIG_HTTP_DEBUG_LEVEL);
	ESP_LOGD(kHttpDebugTag, "Debug log test");
	ESP_LOGV(kHttpDebugTag, "Verbose log test");

	// WARNING: It only works if we have limitation of clients number set to 1. Otherwise, counting mechanism must be implemented
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connectHandler, &server));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnectHandler, &server));

	server = startWebserver();
}
