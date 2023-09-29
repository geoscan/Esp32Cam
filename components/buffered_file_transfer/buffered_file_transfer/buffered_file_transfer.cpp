//
// buffered_file_transfer.hpp
//
// Created: 2023-05-18
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_BUFFERED_FILE_TRANSFER_DEBUG_LEVEL)
#include <esp_log.h>

#ifdef CONFIG_BUFFERED_FILE_TRANSFER_ENABLE
#include "buffered_file_transfer.hpp"
#include "buffered_file_transfer/BufferedFileTransfer.hpp"
#include "buffered_file_transfer/process/SaluteFlashMemoryTransferImplementor.hpp"
#include "buffered_file_transfer/storage/FlushingBufferFileSystem.hpp"
#include "buffered_file_transfer/storage/SaluteFlashMemoryPartitionTable.hpp"
#include "buffered_file_transfer/storage/SingleBufferRamFileSystem.hpp"
#include "system/middleware/FlashMemory.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include "utility/cont/DelayedInitialization.hpp"
#include <sdkconfig.h>
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_ENABLE

namespace Bft {

static constexpr const char *kLogPreamble = "buffered_file_transfer";

void init()
{
#ifdef CONFIG_BUFFERED_FILE_TRANSFER_ENABLE
	esp_log_level_set(Bft::kDebugTag, (esp_log_level_t)CONFIG_BUFFERED_FILE_TRANSFER_DEBUG_LEVEL);
	ESP_LOGD(Bft::kDebugTag, "Debug log test");
	ESP_LOGV(Bft::kDebugTag, "Verbose log test");

	// Initialize the buffer
#if CONFIG_BUFFERED_FILE_TRANSFER_FILE_BUFFER_FILE_SYSTEM_MALLOC_RAM
	static SingleBufferRamFileSystem fileSystem{};
#elif CONFIG_BUFFERED_FILE_TRANSFER_FILE_BUFFER_FILE_SYSTEM_MALLOC_RAM_FLUSHING
	static Ut::Cont::DelayedInitialization<SingleBufferRamFileSystem> bufferFileSystem{};
	static Ut::Cont::DelayedInitialization<FlushingBufferFileSystem> fileSystem{};
	bufferFileSystem.initialize();
	fileSystem.initialize(bufferFileSystem.getInstance());
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_MEMORY_PROVIDER_MALLOC

	// Instantiate transfer implementors
#if CONFIG_BUFFERED_FILE_TRANSFER_IMPLEMENTOR_SALUTE_FLASH
	static Ut::Cont::DelayedInitialization<const SaluteFlashMemoryPartitionTable> saluteFlashMemoryPartitionTable{};
	saluteFlashMemoryPartitionTable.initialize();
	static Ut::Cont::DelayedInitialization<SaluteFlashMemoryTransferImplementor> saluteFlashMemoryTransferImplementor{};
	saluteFlashMemoryTransferImplementor.initialize(static_cast<Sys::FlashMemory *>(nullptr),
		saluteFlashMemoryPartitionTable.getInstance());

	if (!Ut::MakeSingleton<Sys::FlashMemory>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s cannot initialize \"Salute\" implementor, no `Sys::FlashMemory` has been initialized",
			kLogPreamble, __func__);
		Sys::panic();
	}

	saluteFlashMemoryTransferImplementor.getInstance()->setFlashMemoryInstance(
		&Ut::MakeSingleton<Sys::FlashMemory>::getInstance());
	TransferImplementor::subscribeInstanceForBufferingUpdates(saluteFlashMemoryTransferImplementor.getInstance());
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_IMPLEMENTOR_SALUTE_FLASH

	// Register an instance of `BufferedFileSystem`
	volatile static Ut::Cont::DelayedInitialization<BufferedFileTransfer> bufferedFileTransfer{};
	bufferedFileTransfer.initialize(*fileSystem.getInstance());

	(void)fileSystem;
	(void)bufferedFileTransfer;

#endif  // CONFIG_BUFFERED_FILE_TRANSFER_ENABLE
}

}  // namespace Bft
