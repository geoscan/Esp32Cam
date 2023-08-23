//
// zd35.hpp
//
// Created: 2023-08-21
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL)
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
#include "spi_flash_chip_zetta.h"

#include "zd35.hpp"

#ifdef CONFIG_ZD35_ENABLED
extern "C" const spi_flash_chip_t *default_registered_chips[] = {
	&esp_flash_chip_generic,
	&esp_flash_chip_generic,
	nullptr,
};
#endif

namespace Zd35 {

static constexpr const char *kLogPreamble = "zd35";

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
	ESP_LOGD(debugTag(), "%s:%s starting transmission", kLogPreamble, __func__);
	const auto transmissionResult = spi_device_transmit(spiDeviceHandle, &spiTransaction);
	ESP_LOGI(debugTag(), "%s:%s got transaction result=[0x%02x,0x%02x] result=%d", kLogPreamble, __func__, rxBuffer[0],
		rxBuffer[1], static_cast<int>(transmissionResult));
}

void probeAsMx35()
{
	periph_module_enable(PERIPH_HSPI_MODULE);
	default_registered_chips[0] = &esp_flash_chip_zetta;
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

	// Initialize SPI flash
	esp_flash_t *espFlash;
	static const esp_flash_spi_device_config_t espFlashSpiDeviceConfig {
		.host_id = kHost,
		.cs_io_num = GPIO_NUM_15,  // CS
		.io_mode = SPI_FLASH_FASTRD,  // SPI mode
		.speed = ESP_FLASH_40MHZ,
		.input_delay_ns = 0,  // input delay, 0 = don't know
		.cs_id = 0,  // CS line id -- XXX what is "line"?
	};
	spi_bus_add_flash_device(&espFlash, &espFlashSpiDeviceConfig);

	// try to init SPI flash -- ESP IDF will try among known vendor numbers to try to find out which one it deals with
	esp_flash_init(espFlash);
}

void init()
{
#ifdef CONFIG_ZD35_ENABLED

	esp_log_level_set(Zd35::debugTag(), (esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL);
	ESP_LOGD(Zd35::debugTag(), "Debug log test");
	ESP_LOGV(Zd35::debugTag(), "Verbose log test");
#endif
}

}  // namespace Zd35
