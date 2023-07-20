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

#include <esp_http_client.h>
#include <esp_log.h>
#include <algorithm>

#include "file.h"

static constexpr const char kHttpBufferSize = 128;

static esp_err_t httpDownloadFileOverHttpGetEventHandler(esp_http_client_event_t *);

static esp_err_t httpDownloadFileOverHttpGetEventHandler(esp_http_client_event_t *)
{
	// TODO:

	return ESP_FAIL;
}

extern "C" esp_err_t httpDownloadFileOverHttpGet(const char *aFileUrl, OnFileChunkCallable aOnFileChunkCallable)
{
	// Init client
	esp_http_client_config_t espHttpClientConfig{};
	espHttpClientConfig.url = aFileUrl;
	espHttpClientConfig.event_handler = httpDownloadFileOverHttpGetEventHandler;
	esp_http_client_handle_t espHttpClientHandle = esp_http_client_init(&espHttpClientConfig);

	// Download file
	const esp_err_t espErr = esp_http_client_perform(espHttpClientHandle);

	if (espErr == ESP_OK) {
		// TODO: handle
	} else {
		// TODO: handle
	}

	// Teardown routines
	esp_http_client_cleanup(espHttpClientHandle);

	return espErr;
}
