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

static void HttpFetchTest::onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData)
{
	static_cast<HttpFetchTest *>(aUserData)->onFileChunkReceived(aChunk, aChunkSize);
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

void HttpFetchTest::onFileChunkReceived(const char *aChunk, size_t aChunkSize)
{
	if (aChunk == nullptr && aChunkSize != 0) {
		// TODO: handle the beginning of write
	} else if (aChunk == nullptr && aChunkSize == 0) {
		// TODO: finalize write
	} else if (aChunk != nullptr && aChunkSize != 0) {
		// TODO: handle next chunk
	} else {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s "File" API unexpected case, ignoring",
			kLogPreamble, __func__);
	}
}

}  // Bft
