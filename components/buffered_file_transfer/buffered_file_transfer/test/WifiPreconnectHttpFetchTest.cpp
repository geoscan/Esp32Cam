//
// WifiPreconnectHttpFetchTest.cpp
//
// Created on: Oct 23, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Logger.hpp"
#include "wifi.h"

#include "WifiPreconnectHttpFetchTest.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "WifiPreconnectHttpFetchTest";

WifiPreconnectHttpFetchTest::WifiPreconnectHttpFetchTest(const char *aWifiSsid, const char *aWifiPassword,
	const char *aFileHttpUrl, const char *aBufferedFileTransferFileName):
	HttpFetchTest(aFileHttpUrl, aBufferedFileTransferFileName),
	wifiSsid{aWifiSsid},
	wifiPassword{aWifiPassword}
{
}

void WifiPreconnectHttpFetchTest::runTest()
{
	// Connect to target network
	constexpr std::size_t knAttempts = 3;
	esp_err_t connectionResult = ESP_FAIL;

	for (std::size_t iAttempt = knAttempts; iAttempt > 0 && connectionResult != ESP_OK; --iAttempt) {
		connectionResult = wifiStaConnect(wifiSsid, wifiPassword, nullptr, nullptr, nullptr);  // Assign IP over DHCP
	}

	if (connectionResult != ESP_OK) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s failed to connect to SSID %s after %d attempts, aborting!", kLogPreamble, __func__, wifiSsid,
			knAttempts);

		return;
	}

	// Run the test itself
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s starting file acquisition attempt", kLogPreamble,
		__func__);
	HttpFetchTest::runTest();
}

}  // Bft
