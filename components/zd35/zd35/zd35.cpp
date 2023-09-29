//
// zd35.hpp
//
// Created: 2023-08-21
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL)

#include "spi_flash_chip_zetta.h"
#include "system/os/Logger.hpp"
#include "system/os/WorkQueue.hpp"
#include "utility/MakeSingleton.hpp"
#include "utility/cont/DelayedInitialization.hpp"
#include "zd35/FlashMemory.hpp"
#include "zd35/zd35_defs.hpp"
#include <driver/periph_ctrl.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_flash.h>
#include <esp_flash_spi_init.h>
#include <esp_intr_alloc.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <hal/gpio_types.h>
#include <hal/spi_flash_types.h>
#include <spi_flash_chip_generic.h>
#include <array>
#include <cstring>
#include <memory>

#include "zd35.hpp"

extern "C" const spi_flash_chip_t *default_registered_chips[];

#if CONFIG_SPI_FLASH_OVERRIDE_CHIP_DRIVER_LIST
const spi_flash_chip_t *default_registered_chips[] = {
	&esp_flash_chip_generic,  // Stub. The system catches "cache disabled" error when accessing "zetta" driver functions during initial bringup
	&esp_flash_chip_generic,
	nullptr,
};
#endif

namespace Zd35 {

/// \brief Parameterizes a "erase / write / read / compare" cycle for memory
/// testing.
struct ReadWriteMemoryTestCase {
	static constexpr std::size_t kBufferSize = 64;

	/// Data to store in memory
	std::array<char, kBufferSize> outputBuffer;

	/// Buffer to store the data read from memory
	std::array<char, kBufferSize> inputBuffer;

	/// Number of erase block to erase
	std::size_t eraseBlockOffset;

	/// Number of page to read
	std::size_t readWriteBlockOffset;

	/// Offset within the page
	std::size_t readWriteBlockInnerOffset;
	bool erase;

	bool isSuccessful() const
	{
		return strcmp(inputBuffer.data(), outputBuffer.data()) == 0;
	}
};

static constexpr const char *kLogPreamble = "zd35";

/// \brief Test function: initializes the correct SPI bus, and makes an attempt
/// to fetch ZD35 chip ID.
static void testInitSpiProbe();

/// \brief Makes a sequential read/write attempt.
/// \pre An instance of `Sys::FlashMemory` must be registered, i.e.
/// `initImpl()` must be called beforehand.
static bool testReadWrite(void *);

/// \brief Same as \sa `testReadWrite`, but w/ pre-erase
/// \warning Use carefully, as flash memory chips only allow a limited number
/// of write / erase cycles
static bool testEraseReadWrite();

/// \brief Runs multiple "erase / write / read / compare" cycles.
bool runReadWriteMemoryTestCases(void *);

static void initImpl();

/// \brief Checks whether the correct verison of `esp_flash_t` has been
/// initialized
static inline bool espFlashCheckInitialized(const esp_flash_t &espFlash);

static void testInitSpiProbe()
{
	periph_module_enable(PERIPH_HSPI_MODULE);
	constexpr auto kHost = SPI2_HOST;

	// Initialize spi
	static const spi_bus_config_t spiConfig {
		.mosi_io_num = GPIO_NUM_13,  // MOSI
		.miso_io_num = GPIO_NUM_12,  // MISO,
		.sclk_io_num = GPIO_NUM_14,  // CLK,
		.data2_io_num = -1,  // DATA 3 (not used)
		.data3_io_num = -1,  // DATA 4 (not used)
		.data4_io_num = -1,  // DATA 5 (not used)
		.data5_io_num = -1,  // DATA 6 (not used)
		.data6_io_num = -1,  // DATA 7 (not used)
		.data7_io_num = -1,  // DATA 7 (not used)
		.max_transfer_sz = 0,  // Use default max transfer size
		.flags = SPICOMMON_BUSFLAG_MASTER,  // Spi flags -- XXX need none?
		.intr_flags = ESP_INTR_FLAG_LEVEL3,  // Interrupt flags  --- XXX need none?
	};
	spi_bus_initialize(kHost, &spiConfig, SPI_DMA_CH1);

	static const spi_device_interface_config_t spiDeviceInterfaceConfig {
		.command_bits = 8,
		.address_bits = 0,
		.dummy_bits = 8,  // expect data after 8 SPI CLK cycles  XXX: are dummy bits included in the address bits?
		.mode = 0,  // CPOL=0 CPHA=0
		.duty_cycle_pos = 0,  // 50/50 clock duty cycle
		.cs_ena_pretrans = 0,  // ignore, not half-duplex mode
		.cs_ena_posttrans = 0,  // pull CS up right after the transmission is made
		.clock_speed_hz = 1000000,
		.input_delay_ns = 0,  // no known need for delay
		.spics_io_num = GPIO_NUM_15,  // GPIO 15 for CS
		.flags = SPI_DEVICE_HALFDUPLEX,
		.queue_size = 1,
		.pre_cb = nullptr,  // no pre-transaction callback
		.post_cb = nullptr,  // no post-transaction callback
	};

	spi_device_handle_t spiDeviceHandle{};
	spi_bus_add_device(kHost, &spiDeviceInterfaceConfig, &spiDeviceHandle);

	// Try to get device id.
	uint8_t rxBuffer[2] = {0};
	spi_transaction_t spiTransaction {
		.flags = 0,
		.cmd = 0x9f,
		.addr = 0,
		.length = 16,
		.rxlength = 16,  // (Manufacturer id, Device id), size in bits
		.user = nullptr,
		.tx_buffer = 0,
		.rx_buffer = static_cast<void *>(&rxBuffer[0]),
	};
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s starting transmission", kLogPreamble, __func__);
	const auto transmissionResult = spi_device_transmit(spiDeviceHandle, &spiTransaction);
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s got transaction result=[0x%02x,0x%02x] result=%d",
		kLogPreamble, __func__, rxBuffer[0], rxBuffer[1], static_cast<int>(transmissionResult));
}

static inline bool espFlashCheckInitialized(const esp_flash_t &espFlash)
{
	if (espFlash.chip_id != Zd35x2ChipId) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s: chip ids do not match, actual=0x%04X expected=0x%04X", kLogPreamble, __func__, espFlash.chip_id,
			Zd35x2ChipId);

		return false;
	}

	return true;
}

static bool testReadWrite(void *aReadWriteMemoryTestCase)
{
	if (!Ut::MakeSingleton<Sys::FlashMemory>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s: memory test failed, `Sys::FlashMemory` instance has not been initialized", kLogPreamble, __func__);

		return false;
	}

	Sys::Logger::write(Sys::LogLevel::Info, debugTag(),
		"%s:%s trying write/read cycle on a memory chip writeBlockSize=%d", kLogPreamble, __func__,
		Ut::MakeSingleton<Sys::FlashMemory>::getInstance().getFlashMemoryGeometry().writeBlockSize);

#if 1
	// Flush the buffer into flash
	const auto writeResult = Ut::MakeSingleton<Sys::FlashMemory>::getInstance().writeBlock(
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->readWriteBlockOffset,
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->readWriteBlockInnerOffset,
		reinterpret_cast<std::uint8_t *>(reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->outputBuffer.data()),
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->outputBuffer.size());

	if (writeResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to write into the chip",
			kLogPreamble, __func__);
		return false;
	}
#endif

#if 1
	// Read the buffer
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s reading from memory page=%d offset=%d",
		kLogPreamble, __func__, reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->readWriteBlockOffset,
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->readWriteBlockInnerOffset);
	const auto readResult = Ut::MakeSingleton<Sys::FlashMemory>::getInstance().readBlock(
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->readWriteBlockOffset,
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->readWriteBlockInnerOffset,
		reinterpret_cast<std::uint8_t *>(reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->inputBuffer.data()),
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->inputBuffer.size());

	if (readResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to read from the chip",
			kLogPreamble, __func__);
	}

	// Ensure null-termination, and read the buffer
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s read buffer from flash(%s)", kLogPreamble, __func__,
		reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->inputBuffer.data());
#endif

	return false;
}

static bool testEraseReadWrite(void *aReadWriteMemoryTestCase)
{
	if (!Ut::MakeSingleton<Sys::FlashMemory>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s cannot run test function, because no `FlashMemory` instance is registered. Aborting", kLogPreamble,
			__func__);

		return false;
	}

	if (reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->erase) {
		Sys::Error error{};
		error = Ut::MakeSingleton<Sys::FlashMemory>::getInstance().eraseBlock(
			reinterpret_cast<ReadWriteMemoryTestCase *>(aReadWriteMemoryTestCase)->eraseBlockOffset);

		if (error.errorCode != Sys::ErrorCode::None) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s could not erase block, aborting");

			return false;
		}
	} else {
		Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s TEST skipping erase", kLogPreamble, __func__);
	}

	testReadWrite(aReadWriteMemoryTestCase);

	return false;
}

static inline void initImpl()
{
	constexpr auto kHost = SPI2_HOST;

	// The memory is on SPI2 bus (a.k.a. HSPI)
	periph_module_enable(PERIPH_HSPI_MODULE);

	// Bypass the cache error
	default_registered_chips[0] = &esp_flash_chip_zetta;

	// Initialize SPI
	static const spi_bus_config_t spiConfig {
		.mosi_io_num = GPIO_NUM_13,  // MOSI
		.miso_io_num = GPIO_NUM_12,  // MISO,
		.sclk_io_num = GPIO_NUM_14,  // CLK,
		.data2_io_num = -1,  // DATA 3 (not used)
		.data3_io_num = -1,  // DATA 4 (not used)
		.data4_io_num = -1,  // DATA 5 (not used)
		.data5_io_num = -1,  // DATA 6 (not used)
		.data6_io_num = -1,  // DATA 7 (not used)
		.data7_io_num = -1,  // DATA 7 (not used)
		.max_transfer_sz = 0,  // Use default max transfer size
		.flags = SPICOMMON_BUSFLAG_MASTER,  // Spi flags -- XXX need none?
		.intr_flags = ESP_INTR_FLAG_LEVEL3,  // Interrupt flags  --- XXX need none?
	};

	{
		const auto result = spi_bus_initialize(kHost, &spiConfig, SPI_DMA_CH1);

		if (result != ESP_OK) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to initialize SPI bus code=%d (%s)",
				kLogPreamble, __func__, result, esp_err_to_name(result));

			return;
		}
	}

	// Initialize SPI flash
	static const esp_flash_spi_device_config_t espFlashSpiDeviceConfig {
		.host_id = kHost,
		.cs_io_num = GPIO_NUM_15,  // CS
		.io_mode = SPI_FLASH_FASTRD,  // SPI mode
		.speed = ESP_FLASH_5MHZ,
		.input_delay_ns = 0,  // input delay, 0 = don't know
		.cs_id = 0,  // CS line id -- XXX what is "line"?
	};
	esp_flash_t *espFlash = nullptr;

	{
		const auto result = spi_bus_add_flash_device(&espFlash, &espFlashSpiDeviceConfig);

		if (result != ESP_OK || espFlash == nullptr) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
				"%s:%s failed to attach flash device to SPI bus driver code=%d (%s)", kLogPreamble, __func__, result,
				esp_err_to_name(result));

			return;
		}
	}

	// try to init SPI flash -- ESP IDF will try among known vendor numbers to try to find out which one it deals with
	{
		const auto result = esp_flash_init(espFlash);

		if (result != ESP_OK) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
				"%s:%s failed to initialize SPI flash device code=%d (%s)", kLogPreamble, __func__, result,
				esp_err_to_name(result));

			return;
		}
	}

	// Check whether the correct chip is on the bus
	if (!espFlashCheckInitialized(*espFlash)) {
		return;
	}


	// Create instance, and register `Sys::FlashMemory` API as a singleton
	static Ut::Cont::DelayedInitialization<Zd35::FlashMemory> zd35FlashMemoryDelayedInitialization{};
	zd35FlashMemoryDelayedInitialization.initialize(espFlash);
	Ut::MakeSingleton<Sys::FlashMemory>::setInstance(*zd35FlashMemoryDelayedInitialization.getInstance());
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s successfully initialized ZD35 SPI flash memory",
		kLogPreamble, __func__);
}

bool runReadWriteMemoryTestCases(void *)
{
	static std::array<ReadWriteMemoryTestCase, 4> readWriteMemoryTestCaseCases {{
		// Erase before performing read/write. On 2Gb devices, the erase
		// operation will affect the first erase sector (64 pages).
		{
			.outputBuffer = {{"Goodbye"}},
			.inputBuffer = {{}},
			.eraseBlockOffset = 0,
			.readWriteBlockOffset = 0,
			.readWriteBlockInnerOffset = 0,
			.erase = false,  // WARNING Please note that erase cycles damage the device
		},
		// Write in the second page. The page is erased, as it resides in the
		// first erase sector.
		{
			.outputBuffer = {{"Hello"}},
			.inputBuffer = {{}},
			.eraseBlockOffset = 0,
			.readWriteBlockOffset = 1,
			.readWriteBlockInnerOffset = 0,
			.erase = false,
		},
		// Write and read with internal offset (not in/form the beginning of
		// the page)
		{
			.outputBuffer = {{"Wazzup"}},
			.inputBuffer = {{}},
			.eraseBlockOffset = 1,
			.readWriteBlockOffset = 67,  // ZD35's erase blocks are 64 pages long
			.readWriteBlockInnerOffset = 2,
			.erase = false,
		},
		// The last page, the last 4 bits
		{
			.outputBuffer = {{"Four"}},
			.inputBuffer = {{}},
			.eraseBlockOffset = 0,
			.readWriteBlockOffset = Zd35x2CapacityPages - 1,  // ZD35's erase blocks are 64 pages long
			.readWriteBlockInnerOffset = Zd35x2PageSize - ReadWriteMemoryTestCase::kBufferSize,
			.erase = false,
		}
	}};

	std::size_t testCaseCounter = 0;

	for (auto &testCase : readWriteMemoryTestCaseCases) {
		testEraseReadWrite(reinterpret_cast<void *>(&testCase));

		if (testCase.isSuccessful()) {
			Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s TEST passed memory test %d/%d", kLogPreamble,
				__func__, testCaseCounter + 1, readWriteMemoryTestCaseCases.size());
		} else {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s TEST failed memory test %d/%d", kLogPreamble,
				__func__, testCaseCounter + 1, readWriteMemoryTestCaseCases.size());
		}

		++testCaseCounter;
	}

	return false;
}

void init()
{
	static bool isInitialized = false;

	if (isInitialized) {
		Sys::Logger::write(Sys::LogLevel::Warning, debugTag(), "%s:%s already initialized, skipping", kLogPreamble,
			__func__);
	}

	isInitialized = true;

#ifdef CONFIG_ZD35_ENABLED
	esp_log_level_set(Zd35::debugTag(), (esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL);
	Sys::Logger::write(Sys::LogLevel::Debug, debugTag(), "Debug log test");
	Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(), "Verbose log test");
	initImpl();
#endif

#if 0
	testInitSpiProbe();
#endif

#if 0
	if (!Ut::MakeSingleton<Sys::WorkQueue>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s testing code requires a work queue instance to be registered", kLogPreamble, __func__);
		assert(false);
	}

	Ut::MakeSingleton<Sys::WorkQueue>::getInstance().pushTask(runReadWriteMemoryTestCases);
#endif
}

}  // namespace Zd35
