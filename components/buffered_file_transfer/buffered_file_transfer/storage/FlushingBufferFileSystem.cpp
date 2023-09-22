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
	bufferFileDescriptor{}
{
}

FileDescriptor FlushingBufferFileSystem::tryOpenFileWriteBinary(const char *aFilePath, std::size_t aFileSizeHint)
{
	// It's only allowed to have 1 session at a time
	if (bufferFileDescriptor.isValid()) {
		return FileDescriptor{};
	}

	if (bufferFileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s no file system has been provided, panicking!",
			kLogPreamble, __func__);
		Sys::panic();

		return FileDescriptor{};
	}

	// Open RAM file
	bufferFileDescriptor = bufferFileSystem->tryOpenFileWriteBinary(aFilePath, aFileSizeHint);

	if (!bufferFileDescriptor.isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to open file %s in buffer file system", kLogPreamble, __func__, aFilePath);

		return FileDescriptor{};
	}

	return bufferFileDescriptor;
}

std::size_t FlushingBufferFileSystem::append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
	std::size_t aBufferSize)
{
	if (!aFileDescriptor.isValid()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s invalid file descriptor", kLogPreamble, __func__);

		return 0;
	}

	// File system only supports 1 file opened at a time
	if (!aFileDescriptor.isEqual(bufferFileDescriptor)) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s unknown file descriptor", kLogPreamble, __func__);

		return 0;
	}

	if (bufferFileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s no file system has been provided, panicking!",
			kLogPreamble, __func__);
		Sys::panic();

		return 0;  // Won't get here
	}

	std::size_t position = 0;

	while (position < aBufferSize) {
		std::size_t nAppended = bufferFileSystem->append(bufferFileDescriptor, &aBuffer[position],
			aBufferSize - position);

		if (nAppended != aBufferSize) {
			// Flush the accumulated content
			TransferImplementor::notifyAllOnFileBufferingFinished(std::shared_ptr<File>(
				new File{bufferFileSystem, bufferFileDescriptor}), false);

			// Reset the cursor position
			bufferFileSystem->seek(bufferFileDescriptor, 0 /* File beginning */);
		}

		position += nAppended;
	}

	return position;
}

std::int32_t FlushingBufferFileSystem::seek(FileDescriptor aFileDescriptor, std::int32_t aOffset,
	int aOrigin = PositionStart)
{
	if (bufferFileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s no file system has been provided, panicking!",
			kLogPreamble, __func__);
		Sys::panic();

		return FileSystem::PositionError;  // Won't get here
	}

	return bufferFileSystem->seek(aFileDescriptor, aOffset, aOrigin);
}

std::size_t FlushingBufferFileSystem::read(FileDescriptor aFileDescriptor, std::uint8_t *aOutBuffer,
	std::size_t aOutBufferSize)
{
	if (bufferFileSystem == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s no file system has been provided, panicking!",
			kLogPreamble, __func__);
		Sys::panic();

		return 0;  // Won't get here
	}

	return bufferFileSystem->read(aFileDescriptor, aOutBuffer, aOutBufferSize);
}

}  // Bft
