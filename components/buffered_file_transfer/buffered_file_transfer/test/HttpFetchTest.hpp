//
// HttpFetchTest.hpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP

namespace Bft {

/// \brief Implements fetching of a file over HTTP, and its further transfer
/// using whatever scheme happens to be employed at a time
class HttpFetchTest {
public:
	/// \brife `aFileHttpUrl`
	HttpFetchTest(const char *aFileHttpUrl);

	void runTest();

private:
	void onFileChunkReceived(const char *aChunk, size_t aChunkSize, void *aUserData);

private:
	const char *fileHttpUrl;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP
