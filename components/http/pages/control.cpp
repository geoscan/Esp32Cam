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

#include "camera_recorder/RecMjpgAvi.hpp"
#include "camera_recorder/RecFrame.hpp"
#include "sd_fat.h"
#include "wifi.h"
#include "utility/time.hpp"
#include "module/ModuleBase.hpp"
#include "esp_wifi.h"
#include "sub/Cam.hpp"
#include "Ov2640.hpp"
#include "http.h"
#include <memory>
#include <algorithm>
#include <esp_http_server.h>
#include <utility>
#include <string>
#include <cJSON.h>
#include <sdkconfig.h>

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
static constexpr const char *kCameraResolution = "camera_frame_size"; // value
static constexpr const char *kHeight = "height";
static constexpr const char *kWidth = "width";
// Command
static constexpr const char *kCommand = "command"; // key
static constexpr const char *kStop = "stop"; // value
static constexpr const char *kStart = "start"; // value
static constexpr const char *kConnect = "connect"; //value
static constexpr const char *kDisconnect = "disconnect"; //value
static constexpr const char *kSetSsid = "setssid";
// JSON response
static constexpr const char *kSuccess = "success";
static constexpr const char *kMessage = "message";

enum Error : esp_err_t {
	Ok = ESP_OK,
	Err = 1,
	ErrArg = 2,
	ErrCam = 3,
	ErrSd = 4,
	ErrIpParse = 5,
	ErrOther = 6,
	OkNoRequest = 7,

	ErrMax,
};

static_assert(ESP_OK == 0, "");
static std::array<std::string, static_cast<std::size_t>(ErrMax)> sErrorMessages {{
	{""},
	{"Unknown error"},
	{"Wrong input argument(s)"},
	{"Camera Error"},
	{"Storage or filesystem error"},
	{"IP parsing error"},
	{""},
	{""},
}};
static const std::string kStringError{"\0\0", 2};

static bool shotFile(const char *);
static Error processPhoto(std::string name);
/// \brief Legacy handling. Checks whether a `std::string` object is a special
/// string sequence.
static bool stringIsError(const std::string string)
{
	return string.size() == 2 && string[0] == '\0' && string[1] == '\0';
}

static struct {
	bool videoRecRunning = false;
} status;

static struct {
	Sub::Cam::ShotFile shotFile;
} key {{shotFile}};

static void wifiDisconnectHandler(Sub::Key::WifiDisconnected::Type)
{
	if (CameraRecorder::RecMjpgAvi::checkInstance()) {
		CameraRecorder::RecMjpgAvi::getInstance().stop();
		status.videoRecRunning = false;
	}
}

static Sub::Key::WifiDisconnected keyWifiDisconnected{wifiDisconnectHandler};

static bool shotFile(const char *aName)
{
	return Ok == processPhoto(aName);
}

/// \brief getArgValueByKey Parses GET request and extracts values corresponding to the key it is provided with
///
/// \param req Request object
/// \param key Get key to search for
///
/// \return `kStringError`, if no such key has been found. Value of the key otherwise.
static std::string getArgValueByKey(httpd_req_t *req, const char *key)
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

	return kStringError;
}

static Error processVideoStream()
{
	return Err;
}

static Error processVideoRecord(std::string command, std::string name)
{
	if (!CameraRecorder::RecMjpgAvi::checkInstance()) {
		ESP_LOGE(kHttpDebugTag, "control: `CameraRecorder` is not initialized");

		return Err;
	}

	name.insert(0, CONFIG_SD_FAT_MOUNT_POINT"/");
	name.append(".avi");
	sdFatInit();

	Ut::waitMs(100);

	ESP_LOGI("[http]", "command: %s, name: %s", command.c_str(), name.c_str());

	if (command == "start" && !status.videoRecRunning) {
		if (CameraRecorder::RecMjpgAvi::getInstance().start(name.c_str())) {
			status.videoRecRunning = true;
			return Ok;
		}
		return Err;
	} else if (command == "stop" && status.videoRecRunning) {
		CameraRecorder::RecMjpgAvi::getInstance().stop();
		status.videoRecRunning = false;
	} else {
		return Err;
	}
	return Ok;
}

static Error processPhoto(std::string name)
{
	if (!CameraRecorder::RecFrame::checkInstance()) {
		ESP_LOGD(kHttpDebugTag, "control: cannot processPhoto, `RecFrame` is not initialized");

		return Err;
	}

	static constexpr const char *kJpg = ".jpg";

	sdFatInit();

	if (name.length() == 0) {
		return ErrArg;
	}

	// <name> -> <mount_point>/<name>.jpg
	name.insert(0, CONFIG_SD_FAT_MOUNT_POINT"/");
	name.append(kJpg);

	return CameraRecorder::RecFrame::getInstance().start(name.c_str()) ? Ok : Err;
}

static Error processWifi(std::string aCommand, std::string aSsid, std::string aPassword, std::string aIp, std::string aGateway, std::string aNetmask)
{
	if (aCommand == kDisconnect) {
		Error error = Error::Err;
		Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Initialized>(false,
			[&error](const Mod::Fld::WriteResp &response)
			{
				if (response.isOk()) {
					error = Ok;
				}
			});

		return error;
	} else if (aCommand == kConnect) {
		const bool useExplicitAddress = (aIp.size() && aGateway.size() && aNetmask.size());
		esp_err_t connResult;
		bool success = false;
		// Write relevant STA module fields
		Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Password>(aPassword,
			[&success](const Mod::Fld::WriteResp &response)
			{
				success = response.isOk();
			});
		Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::StringIdentifier>(aSsid,
			[&success](const Mod::Fld::WriteResp &response)
			{
				success = success && response.isOk();
			});

		// Write IP network configurations, if needed
		if (useExplicitAddress) {
			ESP_LOGD(kHttpDebugTag, "control: setting IP config-s");
			std::array<asio::error_code, 3> arrErr;
			std::uint32_t ip = asio::ip::make_address_v4(aIp, arrErr[0]).to_uint();
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip>(ip,
				[&success](const Mod::Fld::WriteResp &response)
				{
					success = success && response.isOk();
				});
			std::uint32_t netmask = asio::ip::make_address_v4(aNetmask, arrErr[1]).to_uint();
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Netmask>(netmask,
				[&success](const Mod::Fld::WriteResp &response)
				{
					success = success && response.isOk();
				});
			std::uint32_t gateway = asio::ip::make_address_v4(aGateway, arrErr[2]).to_uint();
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Gateway>(gateway,
				[&success](const Mod::Fld::WriteResp &response)
				{
					success = success && response.isOk();
					ESP_LOGD(kHttpDebugTag, "control: set gateway ip, success=%d", success);
				});
		}

		// Connect
		ESP_LOGD(kHttpDebugTag, "control: connecting to Wi-Fi AP");
		Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Initialized>(true,
			[&success](const Mod::Fld::WriteResp &response)
			{
				success = success && response.isOk();
			});

		if (success) {
			connResult = ESP_OK;
		} else {
			connResult = ESP_FAIL;
		}

		switch (connResult) {
			case ESP_OK:
				return Ok;

			default:
				return Err;
		}
	} else if (aCommand == kSetSsid) {
		auto result = Ok;

		if (!stringIsError(aSsid)) {
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier>(aSsid,
				[&result](const Mod::Fld::WriteResp &response)
				{
					if (!response.isOk()) {
						result = ErrOther;
						sErrorMessages[result] = response.resultAsCstr();
					}
				});
		}

		if (result == Ok && !stringIsError(aPassword)) {
			Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::WifiAp, Mod::Fld::Field::Password>(aPassword,
				[&result](const Mod::Fld::WriteResp &response)
				{
					if (!response.isOk()) {
						result = ErrOther;
						sErrorMessages[result] = response.resultAsCstr();
					}
				});
		}

		return result;
	} else {
		return ErrArg;
	}
}

static Error processCameraResolution(std::string aWidth, std::string aHeight)
{
	Error err = Ok;
	int width = atoi(aWidth.c_str());
	int height = atoi(aHeight.c_str());

	if (0 == width || 0 == height) {
		err = ErrArg;
	} else {
		Mod::ModuleBase::moduleFieldWriteIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>({width, height},
			[&err](Mod::Fld::WriteResp aWriteResp)
			{
				if (Mod::Fld::RequestResult::Ok == aWriteResp.result) {
					err = Ok;
				} else {
					err = ErrOther;
					sErrorMessages[ErrOther] = Mod::Fld::RequestResult::toCstr(aWriteResp.result);
				}
			});
	}

	return err;
}

static void printStatus(httpd_req_t *req, Error res)
{
	auto *root = cJSON_CreateObject();

	cJSON_AddItemToObject(root, kVideoRecord, cJSON_CreateBool(status.videoRecRunning));

	{  // Acquire STA connection info (ESP is a STA to some remote WiFi Access Point)
		bool wifiStaConnected = false;
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::WifiStaConnection,
			Mod::Fld::Field::Initialized>([&wifiStaConnected](bool a) {wifiStaConnected |= a;});
		cJSON_AddItemToObject(root, kWifiStaConnected, cJSON_CreateBool(wifiStaConnected));
		Mod::ModuleBase::moduleFieldReadIter<Mod::Module::WifiStaConnection, Mod::Fld::Field::Ip>(
			[root](std::uint32_t aAddr)
			{
				static constexpr std::size_t kIpLen = 4;
				const auto bytes = asio::ip::address_v4{aAddr}.to_bytes();
				const int bytesInt[kIpLen] = {bytes[3], bytes[2], bytes[1], bytes[0]};
				cJSON_AddItemReferenceToObject(root, kWifiStaIp, cJSON_CreateIntArray(bytesInt, kIpLen));
			});
	}
	// Get camera frame size
	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::Camera, Mod::Fld::Field::FrameSize>(
		[root](const std::pair<int, int> aFrameSize)
		{
			int data[2] = {std::get<0>(aFrameSize), std::get<1>(aFrameSize)};
			cJSON_AddItemReferenceToObject(root, kCameraResolution, cJSON_CreateIntArray(data, 2));
		});

	if (res != OkNoRequest) {
		// Produce response to the request
		cJSON_AddItemToObject(root, kSuccess, cJSON_CreateBool(static_cast<int>(res == Ok)));
	}

	Mod::ModuleBase::moduleFieldReadIter<Mod::Module::WifiAp, Mod::Fld::Field::Mac>(
		[root](std::array<std::uint8_t, 6> *mac)
		{
			std::array<int, 6> macInt;
			std::copy(mac->begin(), mac->end(), macInt.begin());
			cJSON_AddItemReferenceToObject(root, "mac", cJSON_CreateIntArray(macInt.data(), macInt.size()));
		});

	if (!Ut::Al::in(res, Ok, OkNoRequest)) {
		cJSON_AddItemReferenceToObject(root, kMessage, cJSON_CreateString(sErrorMessages[res].c_str()));
	}

	char *json = cJSON_Print(root);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, json, strlen(json));
	free(json);
	cJSON_Delete(root);
}

extern "C" esp_err_t controlHandler(httpd_req_t *req)
{
	Error ret = OkNoRequest;

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
		} else if (value == kCameraResolution) {
			ret = processCameraResolution(getArgValueByKey(req, kWidth), getArgValueByKey(req, kHeight));
		} else {
			ret = ErrArg;
		}
	}

	printStatus(req, ret);

	return ESP_OK;
}
