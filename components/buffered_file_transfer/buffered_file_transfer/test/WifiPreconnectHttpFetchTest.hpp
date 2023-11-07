//
// WifiPreconnectHttpFetchTest.hpp
//
// Created on: Oct 23, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TEST_WIFIPRECONNECTHTTPFETCHTEST_HPP
#define BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TEST_WIFIPRECONNECTHTTPFETCHTEST_HPP

#include "buffered_file_transfer/test/HttpFetchTest.hpp"

namespace Bft {

/// \brief Same as `HttpFetchTest`, but with an additional stage being
/// establishing Wi-Fi connection with a predefined AP using the credentials
/// provided.
class WifiPreconnectHttpFetchTest : public HttpFetchTest {
public:
	WifiPreconnectHttpFetchTest(const char *aWifiSsid, const char *aWifiPassword, const char *aFileHttpUrl,
		const char *aBufferedFileTransferFileName);
	void runTest();

private:
	const char *wifiSsid;
	const char *wifiPassword;
};

}  // Bft

#endif // BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_TEST_WIFIPRECONNECTHTTPFETCHTEST_HPP
