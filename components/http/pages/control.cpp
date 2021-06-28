//
// camera.cpp
//
// Created on: Feb 15, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <memory>
#include <esp_http_server.h>
#include <utility>
#include <string>
#include <cJSON.h>
#include <sdkconfig.h>
#include <Ov2640.hpp>
#include "camera_recorder/RecMjpgAvi.hpp"
#include "camera_recorder/RecFrame.hpp"
#include "sd_fat.h"
#include "utility/time.hpp"

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
static constexpr const char *kMessage = "message";

static struct {
	bool videoRecRunning = false;
} status;

static struct {
	CameraRecorder::RecMjpgAvi mjpgAvi;
	CameraRecorder::RecFrame   frame;
} rec;

static void wifiDisconnectHandler(Utility::Subscription::Key::WifiDisconnected::Type)
{
	rec.mjpgAvi.stop();
	status.videoRecRunning = false;
}

static Utility::Subscription::Key::WifiDisconnected keyWifiDisconnected{wifiDisconnectHandler};

enum Error : esp_err_t {
	// Standard ESP's errors
	Ok      = ESP_OK,
	Err     = ESP_FAIL,
	ErrArg  = ESP_ERR_INVALID_ARG,

	// Custom errors
	ErrNone = ESP_FAIL - 1,
	ErrCam  = ESP_FAIL - 2,
	ErrSd   = ESP_FAIL - 3,
};

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
	if (httpd_query_key_value(urlQuery, key, valueBuf, sizeof(valueBuf)) == Ok)
	{
		return valueBuf;
	}

	return {};
}

static Error processVideoStream()
{
	return Err;
}

static Error processVideoRecord(string command, string name)
{
	name.insert(0, CONFIG_SD_FAT_MOUNT_POINT"/");
	name.append(".avi");
	sdFatInit();

	Utility::waitMs(100);

	ESP_LOGI("[http]", "command: %s, name: %s", command.c_str(), name.c_str());

	if (command == "start" && !status.videoRecRunning) {
		if (rec.mjpgAvi.start(name.c_str())) {
			status.videoRecRunning = true;
			return Ok;
		}
		return Err;
	} else if (command == "stop" && status.videoRecRunning) {
		rec.mjpgAvi.stop();
		status.videoRecRunning = false;
	} else {
		return Err;
	}
	return Ok;
}

static Error processPhoto(string name)
{
	static constexpr const char *kJpg = ".jpg";

	sdFatInit();

	if (name.length() == 0) {
		return ErrArg;
	}

	// <name> -> <mount_point>/<name>.jpg
	name.insert(0, CONFIG_SD_FAT_MOUNT_POINT"/");
	name.append(kJpg);

	return rec.frame.start(name.c_str()) ? Ok : Err;
}

static void printStatus(httpd_req_t *req, Error res)
{
	auto *root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, kVideoRecord, cJSON_CreateBool(status.videoRecRunning));

	if (res != ErrNone) {
		cJSON_AddItemToObject(root, kSuccess, cJSON_CreateBool((int)(res == Ok)));
		if (res != Ok) {
			switch (res) {

				case ErrCam:
					cJSON_AddItemReferenceToObject(root, kMessage, cJSON_CreateString("Camera Error"));
					break;

				case ErrSd:
					cJSON_AddItemReferenceToObject(root, kMessage, cJSON_CreateString("Storage or filesystem error"));
					break;

				case ErrArg:
					cJSON_AddItemReferenceToObject(root, kMessage, cJSON_CreateString("Wrong input argument(s)"));
					break;

				default:
					cJSON_AddItemReferenceToObject(root, kMessage, cJSON_CreateString("Unknown error"));
					break;
			}
		}
	}

	char *json = cJSON_Print(root);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, json, strlen(json));

	free(json);
	cJSON_Delete(root);
}

extern "C" esp_err_t controlHandler(httpd_req_t *req)
{
	Error ret = ErrNone;

	auto value = getArgValueByKey(req, kFunction);

	if (value.length() != 0) {
		if (value == kVideoStream) {
			ret = processVideoStream();
		} else if (value == kVideoRecord) {
			ret = processVideoRecord(getArgValueByKey(req, kCommand), getArgValueByKey(req, kName));
		} else if (value == kPhoto) {
			ret = processPhoto(getArgValueByKey(req, kName));
		}
	}

	printStatus(req, ret);

	return ESP_OK;
}