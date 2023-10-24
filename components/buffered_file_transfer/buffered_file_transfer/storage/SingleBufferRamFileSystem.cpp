//
// SingleBufferRamFileSystem.cpp
//
// Created on: Oct 23, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "system/os/Logger.hpp"
#include "utility/al/Algorithm.hpp"
#include <algorithm>

#include "SingleBufferRamFileSystem.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "SingleBufferRamFileSystem";

FileDescriptor SingleBufferRamFileSystem::tryOpenFileWriteBinary(const char *aFileName, std::size_t aFileSizeHint)
{
	if (memoryPool != nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s only one file is supported, ignoring", kLogPreamble,
			__func__);

		return FileDescriptor{nullptr};
	}

	if (aFileSizeHint == 0) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s invalid file size: %d", kLogPreamble,
			__func__, aFileSizeHint);

		return FileDescriptor{nullptr};
	}

	memoryPool = new std::uint8_t[aFileSizeHint];

	if (memoryPool == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to allocate %d B for file \"%s\"",
			kLogPreamble, __func__, aFileSizeHint, aFileName);
	} else {
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s successfully allocated %d B for file \"%s\"",
			kLogPreamble, __func__, aFileSizeHint, aFileName);
		memoryPoolSize = aFileSizeHint;
	}

	return FileDescriptor{static_cast<void *>(memoryPool)};
}

void SingleBufferRamFileSystem::closeFile(FileDescriptor aFileDescriptor)
{
	if (aFileDescriptor.raw == memoryPool && aFileDescriptor.isValid()) {
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s closing file: deallocating %d B",
			kLogPreamble, __func__, memoryPoolSize);
		delete [] memoryPool;
		memoryPoolSize = 0;
		memoryPoolNextWritePosition = 0;
		memoryPoolCurrentPosition = 0;
	} else {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s unrecognized file, ignoring", kLogPreamble,
			__func__);
	}
}

std::size_t SingleBufferRamFileSystem::append(FileDescriptor aFileDescriptor, const uint8_t *aBuffer,
	std::size_t aBufferSize)
{
	// Ensure end of write does not exceed memory pool -- clamp buffer size
	aBufferSize = Ut::Al::clamp<std::size_t>(aBufferSize, 0, memoryPoolSize - memoryPoolCurrentPosition);

	// Perform write
	std::copy_n(aBuffer, aBufferSize, memoryPool + memoryPoolCurrentPosition);

	// Update current position
	memoryPoolCurrentPosition += aBufferSize;

	// Update write position
	memoryPoolNextWritePosition = memoryPoolCurrentPosition;

	return aBufferSize;
}

int32_t SingleBufferRamFileSystem::seek(FileDescriptor aFileDescriptor, int32_t aOffset, int aOrigin)
{
	// Validate the descriptor
	if (aFileDescriptor.raw != static_cast<void *>(memoryPool)) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s could not recognize file descriptor",
			kLogPreamble, __func__);

		return PositionError;
	}

	// Initialize `newPosition`
	std::int32_t newPosition = 0;

	switch (aOrigin) {
		case PositionCurrent:
			newPosition = static_cast<std::int32_t>(memoryPoolCurrentPosition);

			break;

		case PositionEnd:
			newPosition = static_cast<std::int32_t>(memoryPoolNextWritePosition);

			break;

		default:
			break;
	}

	newPosition += aOffset;
	newPosition = Ut::Al::clamp<std::size_t>(newPosition, 0, static_cast<std::int32_t>(memoryPoolNextWritePosition));

	// Update current position
	const auto cachedMemoryPoolCurrentPosition = static_cast<std::int32_t>(memoryPoolCurrentPosition);  // Previous position
	memoryPoolCurrentPosition = static_cast<std::int32_t>(newPosition);

	return cachedMemoryPoolCurrentPosition;  // Previous position
}

std::size_t SingleBufferRamFileSystem::read(FileDescriptor aFileDescriptor, uint8_t *aOutBuffer,
	std::size_t aOutBufferSize)
{
	// Validate the descriptor
	if (aFileDescriptor.raw != static_cast<void *>(memoryPool)) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s could not recognize file descriptor",
			kLogPreamble, __func__);

		return 0;
	}

	// Ensure that `aOutBufferSize` does not exceed the boundaries
	aOutBufferSize = std::min<std::size_t>(aOutBufferSize, memoryPoolNextWritePosition - memoryPoolCurrentPosition);

	// Read from buffer
	std::copy_n(memoryPool + memoryPoolCurrentPosition, aOutBufferSize, aOutBuffer);

	// Advance current position
	memoryPoolCurrentPosition += aOutBufferSize;

	return aOutBufferSize;
}

}  // Bft
