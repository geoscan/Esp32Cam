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
#include <cstring>
#include <memory>

#include "zd35.hpp"

#ifdef CONFIG_ZD35_ENABLED
extern "C" const spi_flash_chip_t *default_registered_chips[] = {
	&esp_flash_chip_generic,  // Stub. The system catches "cache disabled" error when accessing "zetta" driver functions during initial bringup
	&esp_flash_chip_generic,
	nullptr,
};
#endif

namespace Zd35 {

static constexpr const char *kLogPreamble = "zd35";
static std::unique_ptr<Sys::FlashMemory> sFlashMemoryInstance;

/// \brief Test function: initializes the correct SPI bus, and makes an attempt
/// to fetch ZD35 chip ID.
static void testInitSpiProbe();

/// \brief Makes a sequential read/write attempt.
/// \pre An instance of `Sys::FlashMemory` must be registered, i.e.
/// `initImpl()` must be called beforehand.
static bool testReadWrite(void *);
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
	Sys::Logger::write(Sys::LogLevel::Info, "%s:%s got transaction result=[0x%02x,0x%02x] result=%d", kLogPreamble, __func__, rxBuffer[0],
		rxBuffer[1], static_cast<int>(transmissionResult));
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

static bool testReadWrite(void *)
{
	if (!Ut::MakeSingleton<Sys::FlashMemory>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s: memory test failed, `Sys::FlashMemory` instance has not been initialized", kLogPreamble, __func__);

		return false;
	}

	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s trying write/read cycle on a memory chip",
		kLogPreamble, __func__);

	// Initialize buffer
	char buffer[Ut::MakeSingleton<Sys::FlashMemory>::getInstance().getFlashMemoryGeometry().writeBlockSize] = {0};
	strcpy(buffer, "Hello");

	// Flush the buffer into flash
	const auto writeResult = Ut::MakeSingleton<Sys::FlashMemory>::getInstance().writeBlock(0, 0,
		reinterpret_cast<std::uint8_t *>(buffer), sizeof(buffer));

	if (writeResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to write into the chip",
			kLogPreamble, __func__);
		return false;
	}

	for (int i = 0; i < 6; ++i) {
		buffer[i] = 0;
	}

	// Read the buffer
	const auto readResult = Ut::MakeSingleton<Sys::FlashMemory>::getInstance().readBlock(0, 0,
		reinterpret_cast<std::uint8_t *>(buffer), sizeof(buffer));

	if (readResult.errorCode != Sys::ErrorCode::None) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s failed to read from the chip",
			kLogPreamble, __func__);
	}

	// Ensure null-termination, and read the buffer
	buffer[sizeof(buffer) - 1] = 0;
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s read buffer from flash(%s)", kLogPreamble, __func__,
		&buffer[0]);

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
	sFlashMemoryInstance = std::unique_ptr<Zd35::FlashMemory>(new Zd35::FlashMemory{espFlash});

	if (sFlashMemoryInstance.get() == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s memory error: failed to create `Zd35::FlashMemory` instance", kLogPreamble, __func__);

		return;
	}

	Ut::MakeSingleton<Sys::FlashMemory>::setInstance(*sFlashMemoryInstance.get());
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(), "%s:%s successfully initialized ZD35 SPI flash memory",
		kLogPreamble, __func__);
}

void init()
{
#ifdef CONFIG_ZD35_ENABLED
	esp_log_level_set(Zd35::debugTag(), (esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL);
	Sys::Logger::write(Sys::LogLevel::Debug, debugTag(), "Debug log test");
	Sys::Logger::write(Sys::LogLevel::Verbose, debugTag(), "Verbose log test");
#endif
	initImpl();

#if 1
	if (!Ut::MakeSingleton<Sys::WorkQueue>::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(),
			"%s:%s testing code requires a work queue instance to be registered", kLogPreamble, __func__);
		assert(false);
	}

	Ut::MakeSingleton<Sys::WorkQueue>::getInstance().pushTask(testReadWrite);
#endif
}

}  // namespace Zd35
