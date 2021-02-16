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

static constexpr const char *kName = "name";  // key
// Function
static constexpr const char *kFunction    = "function";      // key
static constexpr const char *kVideoStream = "video_stream";  // value
static constexpr const char *kVideoRecord = "video_record";  // value
static constexpr const char *kPhoto       = "photo";         // value
// Command
static constexpr const char *kCommand = "command";  // key
static constexpr const char *kStop    = "stop";     // value
static constexpr const char *kStart   = "start";    // value
// JSON response
static constexpr const char *kSuccess = "success";

static constexpr esp_err_t kEspErrorNone = ESP_FAIL - 1;  // Workaround to provide 3-state bool for printStatus(2)

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

static esp_err_t printStatus(httpd_req_t *req, esp_err_t resp)
{
	esp_err_t res = kEspErrorNone;
	auto *root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, kVideoStream, cJSON_CreateFalse());
	cJSON_AddItemToObject(root, kVideoRecord, cJSON_CreateFalse());

	if (resp != kEspErrorNone) {
		cJSON_AddItemToObject(root, kSuccess, cJSON_CreateBool((int)(resp == ESP_OK)));
	}

	char *json = cJSON_Print(root);

	httpd_resp_set_type(req, "application/json");
	res = httpd_resp_send(req, json, strlen(json));

	free(json);
	cJSON_Delete(root);

	return res;
}

extern "C" esp_err_t cameraHandler(httpd_req_t *req)
{
	esp_err_t ret = kEspErrorNone;

	auto value = getArgValueByKey(req, kFunction);

	if (value.length() != 0) {
		if (value == kVideoStream) {
			ret = processVideoStream(req);
		} else if (value == kVideoRecord) {
			ret = processVideoRecord(req);
		} else if (value == kPhoto) {
			ret = processPhoto(req);
		}
	}

	printStatus(req, ret);

	return ret;
}