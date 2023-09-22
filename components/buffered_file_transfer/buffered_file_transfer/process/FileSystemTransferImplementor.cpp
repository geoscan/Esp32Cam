//
// FileSystemTransferImplementor.cpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"

#include "FileSystemTransferImplementor.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "FileSystemTransferImplementor";

FileSystemTransferImplementor::FileSystemTransferImplementor(FileSystem *aFileSystem):
	fileSystem{aFileSystem},
	fileDescriptor{}
{
}

void FileSystemTransferImplementor::onFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk)
{
	// Check input data
	if (aFile.get() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s nullptr file, panicking!", kLogPreamble, __func__);

		Sys::panic();
	}

	if (fileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(),
			"%s:%s flash memory file system  has not been initialized. Ignoring", kLogPreamble, __func__);

		return;
	}

	// Open file, if hasn't yet
	if (!fileDescriptor.isValid()) {
		fileDescriptor = fileSystem->tryOpenFileWriteBinary(const char *aFilePath,
			0 /* We don't know its size, and it should not matter */);
	}

	// Try to file into flash memory, chunk-by-chunk
	const std::size_t fileSize = aFile->getSize();
	static constexpr std::size_t kTemporaryBufferSize = 128;
	aFile->seek(0);

	for (std::size_t nFlushed = 0; nFlushed < fileSize;) {
		std::uint8_t temporaryBuffer[kTemporaryBufferSize] = {0};

		// Read from file into temp. buffer
		std::size_t nRead = aFile->read(temporaryBuffer, kTemporaryBufferSize);

		if (nRead == 0) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to read from file, panicking!",
				kLogPreamble, __func__);
			Sys::panic();
		}

		// Flush temp. buffer into flash memory
		const auto nWritten = fileSystem->append(fileDescriptor, temporaryBuffer, nRead);

		if (nWritten != nRead) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
				"%s:%s failed to flush %dB from temporary buffer, flushed %dB instead. Unexpected behavior, panicking!",
				kLogPreamble, __func__, nRead, nWritten);
			Sys::panic();
		}

		nFlushed += nWritten;
	}
}

}  // Bft
