//
// get_ap_name.c
//
// Created on:  Sep 15, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// The following code is just an ad-hoc, simple workaround
// intended to acquire AP name (SSID). It does the following:
// 1. Initializes UART
// 2. Sends a special request
// 3. Waits for a specially-formatted response during a specified timeout
// 4a. If the request's been acquired during that period, sets unique SSID
// 4b. Sets Default SSID otherwise
// 5. Deinitializes UART

#include <esp_timer.h>
#include <esp_system.h>

#include <algorithm>
#include <numeric>
#include <utility>
#include <array>
#include <cstring>
#include "sdkconfig.h"

#include "utility/system/UartDevice.hpp"

#define ESP_WIFI_SSID    CONFIG_ESP_WIFI_SSID
#define SSID_MAX_LENGTH  32
#define MAC_LENGTH       6

using namespace std;

static const unsigned kIdLen = 8;
static char id[kIdLen] = {0};
static const char *prefix = "abcfed123654"; // '\0' is NOT considered as a part

static bool parseResponse(const char *data, const unsigned len)
{
	static const char  *prefixPos = prefix;
	const char * const prefixEnd  = prefix + strlen(prefix);
	static char        *idPos     = id;
	const char * const idEnd      = id + kIdLen;
	const char         *dataPos   = data;
	const char * const dataEnd    = data + len;

	bool res = false;

	if (data && len) {
		// Match prefix
		for(; prefixPos != prefixEnd && dataPos != dataEnd; dataPos++) {
			if (*dataPos == 0) {
				continue;
			}
			if (*dataPos == *prefixPos) {
				prefixPos++;
			} else if (*dataPos == *prefix) {
				prefixPos = prefix + 1;
			} else {
				prefixPos = prefix;
			}
		}

		// Copy Id, if input mathes prefix
		for (; prefixPos == prefixEnd && idPos != idEnd && dataPos != dataEnd; dataPos++, idPos++) {
			*idPos = *dataPos;
		}

		res = (idPos == idEnd);
	}

	return res;
}


extern "C" void getApNameSuffix(uint8_t **data, unsigned *len)
{
	const unsigned kWaitTimeoutMs = 10000;
	Utility::Sys::UartDevice uart(UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, 2000000, UART_PARITY_DISABLE, UART_STOP_BITS_1);
	char buf[256] = {0};
	auto timeEndUs = esp_timer_get_time() + kWaitTimeoutMs * 1000;
	bool parsed = false;

	*data = NULL;
	*len  = 0;

//	uart.write(prefix, sizeof(prefix));
	size_t nRead = uart.read(buf, sizeof(buf));
	while (esp_timer_get_time() < timeEndUs && !(parsed = parseResponse(buf, nRead))) {
		nRead = uart.read(buf, sizeof(buf));
	}

	if (parsed) {
		*data = (uint8_t *)id;
		*len  = kIdLen;
	}
}
