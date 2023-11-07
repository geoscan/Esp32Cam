//
// HttpFetch.cpp
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

#include "HttpFetch.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "HttpFetch";

int HttpFetch::onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData)
{
	static_cast<HttpFetch *>(aUserData)->onFileChunkReceived(aChunk, aChunkSize);

	return ESP_OK;
}

void HttpFetch::fetchFile(const char *aFileHttpUrl, const char *aBufferedFileTransferFileName)
{
	Sys::Logger::write(Sys::LogLevel::Debug, debugTag(), "%s:%s", kLogPreamble, __func__);

	if (!BufferedFileTransfer::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s `BufferedFileTransfer` instance has not been initialized, panicking...", kLogPreamble, __func__);
		Sys::panic();
	}

	static constexpr std::size_t kBufferSizeHint = 20 * 1024;
	bftFile = std::shared_ptr<Bft::File>{new Bft::File{BufferedFileTransfer::getInstance().tryOpenFileWriteBinary(
		aBufferedFileTransferFileName, kBufferSizeHint)},
		// deleter
		[](Bft::File *aFile) mutable
		{
			aFile->close();
			delete aFile;
		}};

	if (!bftFile->isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s Failed to open file %s", kLogPreamble, __func__,
			aBufferedFileTransferFileName);
		Sys::panic();
	}

	Sys::Logger::write(Sys::LogLevel::Debug, debugTag(), "%s:%s starting file acquisition", kLogPreamble, __func__);
	const auto fileGetResult = httpDownloadFileOverHttpGetByUrl(aFileHttpUrl, onFileChunkReceivedWrapper,
		static_cast<void *>(this));

	if (fileGetResult != ESP_OK) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s failed to retrieve a file", kLogPreamble,
			__func__);
	}
}

void HttpFetch::onFileChunkReceived(const char *aChunk, size_t aChunkSize)
{
	Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(), "%s:%s got chunk, is nullptr = %d, chunk size = %d",
		kLogPreamble, __func__, static_cast<int>(aChunk == nullptr), static_cast<int>(aChunkSize));

	if (aChunk == nullptr && aChunkSize != 0) {  // Announcing file size
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s receiving file %d bytes", kLogPreamble, __func__,
			aChunkSize);
	} else if (aChunk == nullptr && aChunkSize == 0) {  // Finalizing reception
		TransferImplementor::notifyAllOnFileBufferingFinished(bftFile, true);
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s finalizing write", kLogPreamble, __func__);

		// Deallocate the resource, trigger file closing
		bftFile.reset();
	} else if (aChunk != nullptr && aChunkSize != 0) {  // Handling new chunk
		const auto nAppended = bftFile->append(reinterpret_cast<const std::uint8_t *>(aChunk), aChunkSize);

		if (nAppended != aChunkSize) {
			Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
				"%s:%s failed to bufferize %d bytes, managed to bufferize %d instead", kLogPreamble, __func__,
				aChunkSize, nAppended);
		}
	} else {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s \"File\" API unexpected case, ignoring",
			kLogPreamble, __func__);
	}
}

}  // Bft