#include "pages/pages.h"

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
		.uri      = "/camera",
		.method   = HTTP_GET,
		.handler  = cameraHandler,
		.user_ctx = 0
	}
};

static const size_t kNpages = sizeof(pages) / sizeof(httpd_uri_t);

static httpd_handle_t startWebserver(void)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

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

	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connectHandler, &server));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnectHandler, &server));

	server = startWebserver();
}
