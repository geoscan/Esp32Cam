//
// HttpFetchTest.cpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "http/client/file.h"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"

#include "HttpFetchTest.hpp"

namespace Bft {

/// \brief Callback. The signature is compliant with HTTP file API (see
/// `file.h`)
static void onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData);

static void onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData)
{
	static_cast<HttpFetchTest *>(aUserData)->onFileChunkReceived(aChunk, aChunkSize, aUserData);
}

HttpFetchTest::HttpFetchTest(const char *aFileHttpUrl, const char *aBufferedFileTransferFileName):
	fileHttpUrl{aFileHttpUrl},
	bufferedFileTransferFileName{aBufferedFileTransferFileName}
{
}

void HttpFetchTest::runTest()
{
	if (!BufferedFileTransfer::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s `BufferedFileTransfer` instance has not been initialized, panicking...", kLogPreamble, __func__);
		Sys::panic();
	}

	bftFile = BufferedFileTransfer::tryOpenFileWriteBinary(bufferedFileTransferFileName);

	if (!bftFile.isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s Failed to open file %s", kLogPreamble, __func__,
			bufferedFileTransferFileName);
		Sys::panic();
	}

	httpDownloadFileOverHttpGetByUrl(fileHttpUrl, onFileChunkReceivedWrapper, static_cast<void *>(this));
}

void HttpFetchTest::onFileChunkReceived(const char *aChunk, size_t aChunkSize, void *aUserData)
{
}

}  // Bft
