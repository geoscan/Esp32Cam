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
#include <cstdlib>

#include "file.h"

static constexpr const char kHttpBufferSize = 128;
static constexpr const char *kDebugContext = "http/file";

/// \brief \sa `httpDownloadFileOverHttpGet`
struct DownloadFileOverHttpContext {
	/// The caller should provide the callback
	OnFileChunkCallable callable;
	void *userData;  // not to be confused w/ `esp_http_client_event_t::user_data`

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
			// TODO: handle return value
			return DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).callable(nullptr, 0,
				DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).userData);  // The last chunk

			break;

		case HTTP_EVENT_ON_DATA:
			// TODO: handle return value
			return DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).callable(
				static_cast<const char *>(aEspHttpClientEvent->data), aEspHttpClientEvent->data_len,
				DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).userData);

			break;

		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGI(kHttpDebugTag, "%s:%s: Successfully connected", kDebugContext, __func__);

			break;

		case HTTP_EVENT_ON_HEADER:
			if (strcmp(aEspHttpClientEvent->header_key, "Content-Length") == 0) {  // Announce content length to the callback
				const int contentLength = atoi(aEspHttpClientEvent->header_value);

				return DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).callable(nullptr,
					contentLength, DownloadFileOverHttpContext::fromEspHttpClientEvent(aEspHttpClientEvent).userData);
			}

			// Produce verbose output, if enabled
#if CONFIG_HTTP_DEBUG_LEVEL == 5
			ESP_LOGV(kHttpDebugTag, "%s:%s: Got header: \"%s\"=\"%s\"", kDebugContext, __func__,
				aEspHttpClientEvent->header_key, aEspHttpClientEvent->header_value);
#endif


			break;

		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(kHttpDebugTag, "%s:%s: disconnected from client", kDebugContext, __func__);

			break;

		case HTTP_EVENT_ERROR:
			ESP_LOGE(kDebugContext, "%s:%s got error", kDebugContext, __func__);

			return ESP_FAIL;

		default:
			return ESP_FAIL;
	}

	return ESP_OK;
}

extern "C" esp_err_t httpDownloadFileOverHttpGetByUrl(const char *aFileUrl, OnFileChunkCallable aOnFileChunkCallable,
	void *aUserData)
{
	// Init client
	esp_http_client_config_t espHttpClientConfig{};
	DownloadFileOverHttpContext downloadFileOverHttpContext {aOnFileChunkCallable, aUserData};
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

extern "C"  esp_err_t httpDownloadFileOverHttpGet(const char *aHost, int aPort, const char *aPath,
	OnFileChunkCallable aOnFileChunkCallable, void *aUserData)
{
	// Init client
	esp_http_client_config_t espHttpClientConfig{};
	DownloadFileOverHttpContext downloadFileOverHttpContext{aOnFileChunkCallable, aUserData};
	espHttpClientConfig.host = aHost;
	espHttpClientConfig.port = aPort;
	espHttpClientConfig.path = aPath;
	espHttpClientConfig.disable_auto_redirect = true;
	espHttpClientConfig.query = "";
	espHttpClientConfig.transport_type = HTTP_TRANSPORT_OVER_TCP;
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
