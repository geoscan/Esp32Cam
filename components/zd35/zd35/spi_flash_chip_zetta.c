// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "zd35/zd35_defs.hpp"
#include <esp_log.h>
#include <hal/spi_flash_hal.h>
#include <spi_flash_chip_generic.h>
#include <stdlib.h>

/* Driver for zetta flash chip */

esp_err_t spi_flash_chip_zetta_probe(esp_flash_t *chip, uint32_t flashId)
{
	if ((flashId & 0xFFFF) != Zd35x2ChipId) {
		return ESP_ERR_NOT_FOUND;
	}

	chip->chip_id = flashId;
	// TODO: extend for 1 Gb versions

	return ESP_OK;
}

esp_err_t spi_flash_chip_issi_set_io_mode(esp_flash_t *chip);
esp_err_t spi_flash_chip_issi_get_io_mode(esp_flash_t *chip, esp_flash_io_mode_t* out_io_mode);

// Use the same implementation as ISSI chips
#define spi_flash_chip_zetta_set_io_mode spi_flash_chip_issi_set_io_mode
#define spi_flash_chip_zetta_get_io_mode spi_flash_chip_issi_get_io_mode
#define spi_flash_chip_zetta_read_reg spi_flash_chip_generic_read_reg

static const char chip_name[] = "zetta";

spi_flash_caps_t spi_flash_chip_zetta_get_caps(esp_flash_t *chip)
{
	return SPI_FLASH_CHIP_CAP_32MB_SUPPORT;
}

// The zetta chip can use the functions for generic chips except from set read mode and probe,
// So we only replace these two functions.
const spi_flash_chip_t esp_flash_chip_zetta = {
	.name = chip_name,
	.timeout = &spi_flash_chip_generic_timeout,
	.probe = spi_flash_chip_zetta_probe,
	.reset = spi_flash_chip_generic_reset,
	.detect_size = spi_flash_chip_generic_detect_size,
	.erase_chip = spi_flash_chip_generic_erase_chip,
	.erase_sector = spi_flash_chip_generic_erase_sector,
	.erase_block = spi_flash_chip_generic_erase_block,
	.sector_size = Zd35x2BlockSize,
	.block_erase_size = Zd35x2BlockSize,

	.get_chip_write_protect = spi_flash_chip_generic_get_write_protect,
	.set_chip_write_protect = spi_flash_chip_generic_set_write_protect,

	.num_protectable_regions = 0,
	.protectable_regions = NULL,
	.get_protected_regions = NULL,
	.set_protected_regions = NULL,

	.read = spi_flash_chip_generic_read,
	.write = spi_flash_chip_generic_write,
	.program_page = spi_flash_chip_generic_page_program,
	.page_size = Zd35x2PageSize,
	.write_encrypted = spi_flash_chip_generic_write_encrypted,

	.wait_idle = spi_flash_chip_generic_wait_idle,
	.set_io_mode = spi_flash_chip_zetta_set_io_mode,
	.get_io_mode = spi_flash_chip_zetta_get_io_mode,

	.read_reg = spi_flash_chip_zetta_read_reg,
	.yield = spi_flash_chip_generic_yield,
	.sus_setup = spi_flash_chip_generic_suspend_cmd_conf,
	.read_unique_id = spi_flash_chip_generic_read_unique_id_none,
	.get_chip_caps = spi_flash_chip_zetta_get_caps,
	.config_host_io_mode = spi_flash_chip_generic_config_host_io_mode,
};
