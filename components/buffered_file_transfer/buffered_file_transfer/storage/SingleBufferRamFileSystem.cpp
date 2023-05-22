//
// SingleBufferRamFileSystem.cpp
//
// Created on: May 22, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "SingleBufferRamFileSystem.hpp"
#include <esp_log.h>
#include <mutex>
#include <stdio.h>

namespace Bft {

class SynchronizedFileDescriptor {
	FileDescriptor fileDescriptor;
	std::mutex mutex;

	inline SynchronizedFileDescriptor()
	{
		resetStateUnsafe();
	}

	/// \brief Makes an attempt to allocate a buffer, and map it onto FS
	bool tryAllocate(std::size_t aFileSize)
	{
		static constexpr const char *kWriteBinaryFileMode = "wb";
		std::lock_guard<std::mutex> lock{mutex};

		// Ensure there is no opened file yet
		if (nullptr != fileDescriptor.raw) {  // Only 1 file is allowed to be opened at a time
			resetStateUnsafe();

			return false;
		}

		// Allocate the buffer
		void *buffer = malloc(aFileSize);

		if (nullptr == buffer) {  // Could not allocate
			resetStateUnsafe();

			return false;
		}

		// Map the buffer into FS
		fileDescriptor.raw = static_cast<void *>(fmemopen(buffer, aFileSize, kWriteBinaryFileMode));

		if (nullptr == fileDescriptor.raw) {
			resetStateUnsafe();

			return false;
		}

		return true;
	}

	/// \brief Closes the file, if opened
	inline void resetStateUnsafe()
	{
		if (nullptr != fileDescriptor.raw) {
			fclose(static_cast<FILE *>(fileDescriptor.raw));
			fileDescriptor.raw = nullptr;
		}
	}
};

static SynchronizedFileDescriptor sSynchronizedFileDescriptor{};

FileDescriptor SingleBufferRamFileSystem::tryOpenFileWriteBinary(const char *, std::size_t aFileSizeHint)
{
	if (sSynchronizedFileDescriptor.tryAllocate(aFileSize)) {
		return sSynchronizedFileDescriptor.fileDescriptor;
	}

	return {nullptr};
}

void SingleBufferRamFileSystem::closeFile(FileDescriptor aFileDescriptor)
{
	std::lock_guard<std::mutex> lock{sSynchronizedFileDescriptor.mutex};
	sSynchronizedFileDescriptor.resetStateUnsafe();
}

std::size_t SingleBufferRamFileSystem::append(FileDescriptor aFileDescriptor, const std::uint8_t *aBuffer,
	std::size_t aBufferSize)
{
	std::lock_guard<std::mutex> lock{sSynchronizedFileDescriptor.mutex};
	const std::size_t nWritten = fwrite(static_cast<const void *>(aBuffer), aBufferSize, sizeof(std::uint8_t),
		static_cast<FILE *>(sSynchronizedFileDescriptor.fileDescriptor.raw));

	return nWritten;
}

}  // namespace Bft
