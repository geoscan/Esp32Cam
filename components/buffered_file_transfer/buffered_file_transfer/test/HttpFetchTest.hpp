//
// HttpFetchTest.hpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP

#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include "buffered_file_transfer/process/HttpFetch.hpp"
#include <memory>

namespace Bft {

/// \brief Implements fetching of a file over HTTP, and its further transfer
/// using whatever scheme happens to be employed at a time
class HttpFetchTest : public HttpFetch {
public:
	/// \brief aFileHttpUrl URL which can be used to retrieve the file
	/// \brief aBufferedFileTransferFileName File that will be references when
	/// using `BufferedFileTransfer` API
	HttpFetchTest(const char *aFileHttpUrl, const char *aBufferedFileTransferFileName);

	void runTest();
private:
	const char *fileHttpUrl;
	const char *bufferedFileTransferFileName;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP