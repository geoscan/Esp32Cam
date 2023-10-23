//
// FlushingBufferFileSystem.cpp
//
// Created on: Sep 21, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"

#include "FlushingBufferFileSystem.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "FlushingBufferFileSystem";

FlushingBufferFileSystem::FlushingBufferFileSystem(FileSystem *aBufferFileSystem):
	bufferFileSystem{aBufferFileSystem},
	bufferFile{}
{
}

FileDescriptor FlushingBufferFileSystem::tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint)
{
	// It's only allowed to have 1 session at a time
	if (bufferFile.get() != nullptr) {
		return FileDescriptor{};
	}

	if (bufferFileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s no file system has been provided, panicking!",
			kLogPreamble, __func__);
		Sys::panic();

		return FileDescriptor{};
	}

	// Open RAM file
	auto bufferFileDescriptor = bufferFileSystem->tryOpenFileWriteBinary(aFilePath, aFileSizeHint);
	bufferFile = std::shared_ptr<File>{new File{bufferFileSystem, bufferFileDescriptor, aFilePath},
		[this](File *aFile) mutable
		{
			Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s closing file", kLogPreamble, __func__);
			closeFile(aFile->getRawFileDescriptor());
		}};

	if (!bufferFileDescriptor.isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to open file %s in buffer file system", kLogPreamble, __func__, aFilePath);

		return FileDescriptor{};
	}

	return bufferFileDescriptor;
}

std::size_t FlushingBufferFileSystem::append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
	std::size_t aBufferSize)
{
	if (!tryAssertFileOperationValid(aFileDescriptor)) {
		return 0;
	}

	std::size_t position = 0;

	while (position < aBufferSize) {
		std::size_t nAppended = bufferFile->append(&aBuffer[position],
			aBufferSize - position);

		if (nAppended != aBufferSize) {  // We've reached the buffer's capacity
			// Flush the accumulated content
			TransferImplementor::notifyAllOnFileBufferingFinished(bufferFile, false);
			Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(),
				"%s:%s the buffer has been flushed into the memory -- resetting the cursor, and continuing",
				kLogPreamble, __func__);

			// Reset the cursor position
			bufferFile->seek(0 /* File beginning */);
		}

		position += nAppended;
	}

	return position;
}

std::int32_t FlushingBufferFileSystem::seek(FileDescriptor aFileDescriptor, std::int32_t aOffset,
	int aOrigin = PositionStart)
{
	if (!tryAssertFileOperationValid(aFileDescriptor)) {
		return FileSystem::PositionError;
	}

	if (bufferFile.get() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s invalid file, panicking!", kLogPreamble, __func__);
		Sys::panic();

		return 0;
	}

	return bufferFile->seek(aOffset, aOrigin);
}

std::size_t FlushingBufferFileSystem::read(FileDescriptor aFileDescriptor, std::uint8_t *aOutBuffer,
	std::size_t aOutBufferSize)
{
	if (!tryAssertFileOperationValid(aFileDescriptor)) {
		return 0;
	}

	return bufferFile->read(aOutBuffer, aOutBufferSize);
}

void FlushingBufferFileSystem::closeFile(FileDescriptor aFileDescriptor)
{
	if (bufferFile.get() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s no opened files have been found, ignoring",
			kLogPreamble, __func__);

		return;
	}

	if (bufferFile->getRawFileDescriptor().isEqual(aFileDescriptor)) {
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s closing file", kLogPreamble, __func__);
		bufferFile->close();
		bufferFile.reset();
	}
}

bool FlushingBufferFileSystem::tryAssertFileOperationValid(FileDescriptor aFileDescriptor) const
{
	if (!aFileDescriptor.isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s invalid file descriptor", kLogPreamble, __func__);

		return false;
	}

	if (bufferFile.get() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s invalid file, panicking!", kLogPreamble, __func__);
		Sys::panic();

		return false;
	}

	// File system only supports 1 file opened at a time
	if (!aFileDescriptor.isEqual(bufferFile->getRawFileDescriptor())) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s unknown file descriptor", kLogPreamble, __func__);

		return false;
	}

	if (bufferFileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s no file system has been provided, panicking!",
			kLogPreamble, __func__);
		Sys::panic();

		return false;
	}

	return true;
}

}  // Bft
