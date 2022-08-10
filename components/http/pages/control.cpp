//
// camera.cpp
//
// Created on: Feb 15, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// API providing client with an access to resources such as:
// * Camera
// * Wifi
//

#include <memory>
#include <algorithm>
#include <esp_http_server.h>
#include <utility>
#include <string>
#include <cJSON.h>
#include <sdkconfig.h>
#include <Ov2640.hpp>
#include "camera_recorder/RecMjpgAvi.hpp"
#include "camera_recorder/RecFrame.hpp"
#include "sd_fat.h"
#include "wifi.h"
#include "utility/time.hpp"
#include "module/ModuleBase.hpp"
#include "esp_wifi.h"
#include "sub/Cam.hpp"

using namespace std;

// Various keys
static constexpr const char *kName = "name"; // key
static constexpr const char *kSsid = "ssid"; // key
static constexpr const char *kPassword = "password"; // key
static constexpr const char *kIp = "ip"; // key
static constexpr const char *kGateway = "gateway"; // key
static constexpr const char *kNetmask = "netmask"; // key
// Function
static constexpr const char *kFunction = "function"; // key
static constexpr const char *kVideoStream = "video_stream"; // value
static constexpr const char *kVideoRecord = "video_record"; // value
static constexpr const char *kPhoto = "photo"; // value
static constexpr const char *kWifi = "wifi"; // value
static constexpr const char *kWifiStaConnected = "wifi_sta_connected"; // value
static constexpr const char *kWifiStaIp = "wifi_sta_ip"; // value
// Command
static constexpr const char *kCommand = "command"; // key
static constexpr const char *kStop = "stop"; // value
static constexpr const char *kStart = "start"; // value
static constexpr const char *kConnect = "connect"; //value
static constexpr const char *kDisconnect = "disconnect"; //value
// JSON response
static constexpr const char *kSuccess = "success";
static constexpr const char *kMessage = "message";

enum Error : esp_err_t {
	// Standard ESP's errors
	Ok      = ESP_OK,
	Err     = ESP_FAIL,
	ErrArg  = ESP_ERR_INVALID_ARG,

	// Custom errors
	ErrNone    = ESP_FAIL - 1,
	ErrCam     = ESP_FAIL - 2,
	ErrSd      = ESP_FAIL - 3,
	ErrIpParse = ESP_FAIL - 4,
};

static bool shotFile(const char *);
static Error processPhoto(string name);

static struct {
	bool videoRecRunning = false;
} status;

static struct {
	CameraRecorder::RecMjpgAvi mjpgAvi;
	CameraRecorder::RecFrame   frame;
} rec;

static struct {
	Sub::Cam::ShotFile shotFile;
} key {{shotFile}};

static void wifiDisconnectHandler(Sub::Key::WifiDisconnected::Type)
{
	rec.mjpgAvi.stop();
	status.videoRecRunning = false;
}

static Sub::Key::WifiDisconnected keyWifiDisconnected{wifiDisconnectHandler};

static bool shotFile(const char *aName)
{
	return Ok == processPhoto(aName);
}

///
/// \brief getArgValueByKey Parses GET request and extracts values corresponding to the key it is provided with
///
/// \param req Request object
/// \param key Get key to search for
///
/// \return "", if no such key has been found. Value of the key otherwise.
///
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

static Error processWifi(string aCommand, string aSsid, string aPassword, string aIp, string aGateway, string aNetmask)
{
	if (aCommand == kDisconnect) {
		esp_wifi_disconnect();
		return Ok;
	} else if (aCommand != kConnect) {
		return ErrArg;
	}

	const bool useExplicitAddress = (aIp.size() && aGateway.size() && aNetmask.size());
	esp_err_t connResult;

	if (useExplicitAddress) {
		std::array<asio::error_code, 3> arrErr;
		asio::ip::address_v4::bytes_type ip = asio::ip::make_address_v4(aIp, arrErr[0]).to_bytes();
		asio::ip::address_v4::bytes_type gateway = asio::ip::make_address_v4(aGateway, arrErr[1]).to_bytes();
		asio::ip::address_v4::bytes_type netmask = asio::ip::make_address_v4(aNetmask, arrErr[2]).to_bytes();

		if (std::any_of(arrErr.begin(), arrErr.end(), [](const asio::error_code &errCode) {return static_cast<bool>(errCode);}))
		{
			return ErrIpParse;
		}

		connResult = wifiStaConnect(aSsid.c_str(), aPassword.c_str(), ip.data(), gateway.data(), netmask.data());
	} else {
		connResult = wifiStaConnect(aSsid.c_str(), aPassword.c_str(), nullptr, nullptr, nullptr);
	}

	switch (connResult) {
		case ESP_OK:
			return Ok;

		default:
			return Err;
	}

}

static void printStatus(httpd_req_t *req, Error res)
{
	auto *root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, kVideoRecord, cJSON_CreateBool(status.videoRecRunning));

	{
		bool wifiStaConnected = false;
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::WifiStaConnection,
			Mod::Fld::Field::Initialized>([&wifiStaConnected](bool a) {wifiStaConnected |= a;});
		cJSON_AddItemToObject(root, kWifiStaConnected, cJSON_CreateBool(wifiStaConnected));
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::WifiStaConnection,
			Mod::Fld::Field::Ip>(
			[root](const mapbox::util::variant<asio::ip::address_v4> &aAddr)
			{
				aAddr.match(
					[root, &aAddr](const asio::ip::address_v4 &aAddr) mutable
					{
						static constexpr auto kIpLen = 4;
						const auto bytes = aAddr.to_bytes();
						const int bytesInt[kIpLen] = {bytes[3], bytes[2], bytes[1], bytes[0]};
						cJSON_AddItemReferenceToObject(root, kWifiStaIp, cJSON_CreateIntArray(bytesInt, kIpLen));
					}
				);
			});
	}

	// Response a request
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

				case ErrIpParse:
					cJSON_AddItemReferenceToObject(root, kMessage, cJSON_CreateString("IP parsing error"));
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
		} else if (value == kWifi) {
			ret = processWifi(getArgValueByKey(req, kCommand),
				getArgValueByKey(req, kSsid),
				getArgValueByKey(req, kPassword),
				getArgValueByKey(req, kIp),
				getArgValueByKey(req, kGateway),
				getArgValueByKey(req, kNetmask));
		}
	}

	printStatus(req, ret);

	return ESP_OK;
}
