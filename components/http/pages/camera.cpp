//
// camera.cpp
//
// Created on: Feb 15, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <esp_http_server.h>
#include <utility>
#include <string>
#include <cJSON.h>

using namespace std;

static const char *kFunction    = "function";
static const char *kVideoStream = "video_stream";
static const char *kVideoRecord = "video_record";
static const char *kPhoto       = "photo";

static string getArgValueByKey(httpd_req_t *req, const char *key)
{
	static constexpr size_t kValueBufSize = 20;

	const size_t urlQueryLength = httpd_req_get_url_query_len(req) + 1;
	if (urlQueryLength == 0) {
		return {};
	}

	char valueBuf[kValueBufSize]  = {0};
	char urlQuery[urlQueryLength] = {0};

	httpd_req_get_url_query_str(req, urlQuery, urlQueryLength);
	if (httpd_query_key_value(urlQuery, key, valueBuf, sizeof(valueBuf)) == ESP_OK)
	{
		return valueBuf;
	}

	return {};
}

static esp_err_t processVideoStream(httpd_req_t *req)
{
	return ESP_OK;
}

static esp_err_t processVideoRecord(httpd_req_t *req)
{
	return ESP_OK;
}

static esp_err_t processPhoto(httpd_req_t *req)
{
	return ESP_OK;
}

static esp_err_t printStatus(httpd_req_t *req)
{
	esp_err_t res = ESP_OK;
	auto *root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, "Stream Control Available", cJSON_CreateFalse());
	cJSON_AddItemToObject(root, "Video Recording Available", cJSON_CreateFalse());
	cJSON_AddItemToObject(root, "Photo available", cJSON_CreateFalse());
	cJSON_AddItemToObject(root, "# Video Stream Sinks", cJSON_CreateNumber(12));

	char *json = cJSON_Print(root);

	httpd_resp_set_type(req, "application/json");
	res = httpd_resp_send(req, json, strlen(json));

	free(json);
	cJSON_Delete(root);

	return res;
}

extern "C" esp_err_t cameraHandler(httpd_req_t *req)
{
	esp_err_t ret = ESP_OK;

	auto value = getArgValueByKey(req, kFunction);

	if (value.length() != 0) {
		if (value == kVideoStream) {
			processVideoStream(req);
		} else if (value == kVideoRecord) {
			processVideoRecord(req);
		} else if (value == kPhoto) {
			processPhoto(req);
		}
	}

	printStatus(req);

	return ret;
}