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
#include "buffered_file_transfer/test/HttpFetchTest.hpp"
#include "system/middleware/FlashMemory.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include "utility/cont/DelayedInitialization.hpp"
#include "wifi.h"
#include <sdkconfig.h>
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_ENABLE

namespace Bft {

static constexpr const char *kLogPreamble = "buffered_file_transfer";

/// \brief Tries the following:
/// 1. Fetch the file from a HTTP server by URL;
/// 2. Transfer the file using BFT API. The exact configuration is hidden from
/// the test
static void testHttpFileFetching();

static void testHttpFileFetching()
{
	static constexpr const char *kFileUrl = "http://192.168.43.1:8080/max_size_2Hz_0.bin";
	static constexpr const char *kBftFileName = "show";
	static constexpr const char *kTargetWifiSsid = "testtest";
	static constexpr const char *kTargetWifiPassword = "beginend";

	// Connect to target network
	const auto connectionResult = wifiStaConnect(kTargetWifiSsid, kTargetWifiPassword,
		nullptr, nullptr, nullptr);  // Assign IP over DHCP

	if (connectionResult != ESP_OK) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to connect to SSID %s, aborting...",
			kLogPreamble, __func__, kTargetWifiSsid);

		return;
	}

	// Run the test itself
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s starting file acquisition attempt", kLogPreamble,
		__func__);
	static HttpFetchTest httpFetchTest{kFileUrl, kBftFileName};
	httpFetchTest.runTest();
}

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
	static SingleBufferRamFileSystem bufferFileSystem{};
	static FlushingBufferFileSystem fileSystem{&bufferFileSystem};
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_MEMORY_PROVIDER_MALLOC

	// Instantiate transfer implementors
#if CONFIG_BUFFERED_FILE_TRANSFER_IMPLEMENTOR_SALUTE_FLASH
	static SaluteFlashMemoryPartitionTable saluteFlashMemoryPartitionTable{};
	static SaluteFlashMemoryTransferImplementor saluteFlashMemoryTransferImplementor{
		static_cast<Sys::FlashMemory *>(nullptr), &saluteFlashMemoryPartitionTable};

	if (!Ut::MakeSingleton<Sys::FlashMemory>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s cannot initialize \"Salute\" implementor, no `Sys::FlashMemory` has been initialized",
			kLogPreamble, __func__);
		Sys::panic();
	}

	saluteFlashMemoryTransferImplementor.setFlashMemoryInstance(&Ut::MakeSingleton<Sys::FlashMemory>::getInstance());
	TransferImplementor::subscribeInstanceForBufferingUpdates(&saluteFlashMemoryTransferImplementor);
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_IMPLEMENTOR_SALUTE_FLASH

	// Register an instance of `BufferedFileSystem`
	static volatile BufferedFileTransfer bufferedFileTransfer{fileSystem};
#endif  // CONFIG_BUFFERED_FILE_TRANSFER_ENABLE

#if 1
	testHttpFileFetching();
#endif
}

}  // namespace Bft
