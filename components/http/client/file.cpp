//
// file.cpp
//
// Created on:  Jul 20, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
// TODO: Naming
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_HTTP_DEBUG_LEVEL)
#include <esp_log.h>

#include "http.h"
#include <esp_http_client.h>
#include <esp_log.h>
#include <algorithm>

#include "file.h"

static constexpr const char kHttpBufferSize = 128;
static constexpr const char *kDebugContext = "http/file";

/// \brief \sa `httpDownloadFileOverHttpGet`
struct DownloadFileOverHttpContext {
	/// The caller should provide the callback
	OnFileChunkCallable callable;

	static inline DownloadFileOverHttpContext &fromEspHttpClientEvent(esp_http_client_event_t *aEspHttpClientEvent)
	{
		return *static_cast<DownloadFileOverHttpContext *>(aEspHttpClientEvent->user_data);
	}
};

static esp_err_t httpDownloadFileOverHttpGetEventHandler(esp_http_client_event_t *);

static esp_err_t httpDownloadFileOverHttpGetEventHandler(esp_http_client_event_t *aEspHttpClientEvent)
{
	switch (aEspHttpClientEvent->event_id) {
		case HTTP_EVENT_ON_FINISH:
			DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).callable(nullptr, 0);  // The last chunk

			break;

		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGI(kHttpDebugTag, "%s:%s: Successfully connected", kDebugContext, __func__);

			break;

		case HTTP_EVENT_ON_HEADER:
			// TODO: check header content

			// Produce verbose output, if enabled
#if CONFIG_HTTP_DEBUG_LEVEL == 5
			ESP_LOGV(kHttpDebugTag, "%s:%s: Got header: \"%s\"=\"%s\"", kDebugContext, __func__, buffer,
				aEspHttpClientEvent->header_key, aEspHttpClientEvent->header_value);
#endif

			break;

		case HTTP_EVENT_ON_DATA:
			DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).callable(
				static_cast<const char *>(aEspHttpClientEvent->data), aEspHttpClientEvent->data_len);

			break;

		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(kHttpDebugTag, "%s:%s: disconnected from client", kDebugContext, __func__);

			break;

		case HTTP_EVENT_ERROR:
			ESP_LOGE(kDebugContext, "%s:%s got error", kDebugContext, __func__);

			break;

		default:
			break;
	}

	return ESP_FAIL;
}

extern "C" esp_err_t httpDownloadFileOverHttpGet(const char *aFileUrl, OnFileChunkCallable aOnFileChunkCallable)
{
	// Init client
	esp_http_client_config_t espHttpClientConfig{};
	DownloadFileOverHttpContext downloadFileOverHttpContext {aOnFileChunkCallable};
	espHttpClientConfig.url = aFileUrl;
	espHttpClientConfig.event_handler = httpDownloadFileOverHttpGetEventHandler;
	espHttpClientConfig.user_data = static_cast<void *>(&downloadFileOverHttpContext);
	esp_http_client_handle_t espHttpClientHandle = esp_http_client_init(&espHttpClientConfig);

	// Download file
	const esp_err_t espErr = esp_http_client_perform(espHttpClientHandle);

	if (espErr == ESP_OK) {
		ESP_LOGV(kHttpDebugTag, "%s:%s: successfully handled HTTP GET request", kDebugContext, __func__);
	} else {
		ESP_LOGE(kHttpDebugTag, "%s:%s: failed to handle HTTP GET request", kDebugContext, __func__);
	}

	// Teardown routines
	esp_http_client_cleanup(espHttpClientHandle);

	return espErr;
}
