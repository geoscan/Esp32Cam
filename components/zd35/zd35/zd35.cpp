//
// zd35.hpp
//
// Created: 2023-08-21
//  Author:
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL)
#include <driver/spi_common.h>
#include <esp_flash.h>
#include <esp_flash_spi_init.h>
#include <esp_log.h>
#include <hal/spi_flash_types.h>

#include "zd35.hpp"

#ifdef CONFIG_ZD35_ENABLED
extern "C" const spi_flash_chip_t esp_flash_chip_zetta;
// Override drivers, see `spi_flash_chip_drivers.c`
extern "C" const spi_flash_chip_t *default_registered_chips[] = {
	&esp_flash_chip_zetta,
	nullptr,
};
#endif

namespace Zd35 {

void probeAsMx35()
{
	// Initialize spi
	static const spi_bus_config_t spi1Config {
		13,  // MOSI
		12,  // MISO,
		14,  // CLK,
		-1,  // DATA 2 (not used)
		-1,  // DATA 3 (not used)
		-1,  // DATA 4 (not used)
		-1,  // DATA 5 (not used)
		-1,  // DATA 6 (not used)
		-1,  // DATA 7 (not used)
		0,  // Use default max transfer size
		0,  // Spi flags -- XXX need none?
		0,  // Interrupt flags  --- XXX need none?
	};
	spi_bus_initialize(SPI1_HOST, &spi1Config, SPI_DMA_DISABLED);

	// Initialize SPI flash
	esp_flash_t *espFlash;
	static const esp_flash_spi_device_config_t espFlashSpiDeviceConfig {
		SPI1_HOST,
		15,  // CS
		SPI_FLASH_FASTRD,  // SPI mode
		ESP_FLASH_40MHZ,
		0,  // input delay, 0 = don't know
		0,  // CS line id -- XXX what is "line"?
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
