//
// HttpFetchTest.cpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "buffered_file_transfer/process/TransferImplementor.hpp"
#include "http/client/file.h"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include <esp_err.h>

#include "HttpFetchTest.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "HttpFetchTest";

int HttpFetchTest::onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData)
{
	static_cast<HttpFetchTest *>(aUserData)->onFileChunkReceived(aChunk, aChunkSize);

	return ESP_OK;
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

	static constexpr std::size_t kBufferSizeHint = 20 * 1024;
	bftFile = std::shared_ptr<Bft::File>{new Bft::File{BufferedFileTransfer::getInstance().tryOpenFileWriteBinary(
		bufferedFileTransferFileName, kBufferSizeHint)},
		// deleter
		[](Bft::File *aFile) mutable
		{
			aFile->close();
			delete aFile;
		}};

	if (!bftFile->isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s Failed to open file %s", kLogPreamble, __func__,
			bufferedFileTransferFileName);
		Sys::panic();
	}

	httpDownloadFileOverHttpGetByUrl(fileHttpUrl, onFileChunkReceivedWrapper, static_cast<void *>(this));
}

void HttpFetchTest::onFileChunkReceived(const char *aChunk, size_t aChunkSize)
{
	if (aChunk == nullptr && aChunkSize != 0) {
		bftFile->append(reinterpret_cast<const std::uint8_t *>(aChunk), aChunkSize);
		TransferImplementor::notifyAllOnFileBufferingFinished(bftFile, false);
	} else if (aChunk == nullptr && aChunkSize == 0) {
		TransferImplementor::notifyAllOnFileBufferingFinished(bftFile, true);
	} else if (aChunk != nullptr && aChunkSize != 0) {
		// TODO: handle next chunk
	} else {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s \"File\" API unexpected case, ignoring",
			kLogPreamble, __func__);
	}
}

}  // Bft
