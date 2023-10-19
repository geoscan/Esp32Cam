//
// HttpFetchTest.hpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP

#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include <memory>

namespace Bft {

/// \brief Implements fetching of a file over HTTP, and its further transfer
/// using whatever scheme happens to be employed at a time
class HttpFetchTest {
public:
	/// \brief aFileHttpUrl URL which can be used to retrieve the file
	/// \brief aBufferedFileTransferFileName File that will be references when
	/// using `BufferedFileTransfer` API
	HttpFetchTest(const char *aFileHttpUrl, const char *aBufferedFileTransferFileName);

	void runTest();

private:
	/// \pre `bftFile` is valid
	void onFileChunkReceived(const char *aChunk, size_t aChunkSize);

	/// \brief Callback. The signature is compliant with HTTP file API (see
	/// `file.h`)
	static void onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData);

private:
	const char *fileHttpUrl;
	const char *bufferedFileTransferFileName;
	std::shared_ptr<Bft::File> bftFile;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_HTTPFETCHTEST_HPP
