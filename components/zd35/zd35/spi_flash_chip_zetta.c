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


/* Driver for zetta flash chip */
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_ZD35_DEBUG_LEVEL)
#include <sdkconfig.h>
#include <esp_log.h>

#include "zd35/zd35_defs.hpp"
#include <esp_log.h>
#include <hal/spi_flash_hal.h>
#include <spi_flash_chip_generic.h>
#include <stdlib.h>

static const char *DEBUG_TAG = "[zd35]";
static const char *LOG_PREAMBLE = "spi_flash_chip_zetta";

/// Returns true if BRWD register is 0, and all blocks on all panes are
/// write-unprotected (TB bit = 1, BPx bits = 0)
static bool block_lock_register_is_write_protected(uint8_t block_lock_register);

/// Performs `GET_FEATURES` operation upon the `chip`: reads the value of a
/// specified register at `register_address`.
/// \pre `out_buffer` size is 1 byte
static esp_err_t spi_flash_chip_zetta_perform_get_features(esp_flash_t *chip, uint8_t register_address, uint8_t *out_buffer);

static esp_err_t spi_flash_chip_zetta_get_write_protect(esp_flash_t *chip, bool *write_protect);
static esp_err_t spi_flash_chip_zetta_set_write_protect(esp_flash_t *chip, bool write_protect);
static esp_err_t spi_flash_chip_zetta_probe(esp_flash_t *chip, uint32_t flashId);
static esp_err_t spi_flash_chip_zetta_read(esp_flash_t *chip, void *buffer, uint32_t address, uint32_t length);
static spi_flash_caps_t spi_flash_chip_zetta_get_caps(esp_flash_t *chip);
static uint8_t get_zd35_full_lock_mask();
static esp_err_t spi_flash_chip_zetta_perform_set_features(esp_flash_t *chip, uint8_t register_address,
	uint8_t register_value);

static esp_err_t spi_flash_chip_zetta_perform_set_features(esp_flash_t *chip, uint8_t register_address,
	uint8_t register_value)
{
	const uint8_t mosi_buffer[] = {register_address, register_value};
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 2,  // register address + value thereof
		.miso_len = 0,
		.address_bitlen = 8,
		.address = Zd35AddressBlockLock,
		.mosi_data = mosi_buffer,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandSetFeatures,
		.dummy_bitlen = 0,
		.io_mode = 0,
	};
	esp_err_t err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	return err;
}

static inline uint8_t get_zd35_full_lock_mask()
{
	return Zd35RegisterBlockLockBrwdMask | Zd35registerBlockLockBp0 | Zd35registerBlockLockBp1
		| Zd35registerBlockLockBp2 | Zd35registerBlockLockBp3 | Zd35registerBlockLockTb;
}

static inline bool block_lock_register_is_write_protected(uint8_t block_lock_register)
{
	int result = (block_lock_register & get_zd35_full_lock_mask());

	return (result != Zd35registerBlockLockTb);
}

static inline esp_err_t spi_flash_chip_zetta_perform_get_features(esp_flash_t *chip, uint8_t register_address, uint8_t *out_buffer)
{
	// Configure the transaction to be made
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 1,  // 1 command byte
		.miso_len = 1,  // 1 output byte
		.address_bitlen = 8,
		.address = Zd35AddressBlockLock,
		.mosi_data = &register_address,
		.miso_data = out_buffer,
		.flags = 0,  // XXX
		.command = Zd35CommandGetFeatures,
		.dummy_bitlen = 0,
		.io_mode = 0,  // XXX
	};

	// Perform the transfer
	esp_err_t err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	return err;
}

// TODO: fdecl
// TODO: description
static inline esp_err_t spi_flash_chip_zetta_perform_page_read(esp_flash_t *chip, uint32_t page_address)
{
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,
		.miso_len = 0,
		.address_bitlen = 24,
		.address = page_address,
		.mosi_data = NULL,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandPageRead,  // TODO
		.dummy_bitlen = 0,
		.io_mode = 0,
	};
	esp_err_t err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	if (err != ESP_OK) {
		return err;
	}

	// Poll the device until the page is read
	for (int i = 99; i; --i) {  // TODO magic num
		uint8_t register_value = 0;
		err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressStatus, &register_value);

		if (err != ESP_OK) {
			return err;
		}

		if (!(register_value & Zd35RegisterStatusOip)) {
			break;
		} else {
			// TODO: yield
		}
	}

	return err;
}

// TODO: fdecl
// TODO: description
static inline esp_err_t spi_flash_chip_zetta_perform_read_from_cache(esp_flash_t *chip, void *buffer, uint32_t cache_offset,
	uint32_t length)
{
	if (cache_offset >= Zd35x2CacheSize || length > Zd35x2CacheSize || cache_offset + length > Zd35x2CacheSize) {
		return ESP_ERR_INVALID_ARG;
	}

	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,
		.miso_len = length,
		.address_bitlen = 24,
		.address = cache_offset,
		.mosi_data = NULL,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandReadFromCache,  // TODO
		.dummy_bitlen = 0,
		.io_mode = 0,
	};
	esp_err_t err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	return err;
}

static esp_err_t spi_flash_chip_zetta_probe(esp_flash_t *chip, uint32_t flashId)
{
	if ((flashId & 0xFFFF) == Zd35x2ChipId) {
		chip->chip_id = flashId;
		chip->size = Zd35x2CapacityBytes;
	} else {
		return ESP_ERR_NOT_FOUND;
	}

	// TODO: extend for 1 Gb versions

	return ESP_OK;
}

static esp_err_t spi_flash_chip_zetta_get_write_protect(esp_flash_t *chip, bool *write_protect)
{
	static const char *register_name = "Block lock";
	uint8_t register_value = 0;
	esp_err_t err = ESP_OK;

	if (chip == NULL || write_protect == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	// Make a transfer
	err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressBlockLock, &register_value);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s: failed to perform GET_FEATURES", LOG_PREAMBLE, __func__);

		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s: %s=%d", LOG_PREAMBLE, __func__, register_name, register_value);

	// Parse the response
	*write_protect = block_lock_register_is_write_protected(register_value);
	ESP_LOGV(DEBUG_TAG, "%s:%s: write_protect=%d", LOG_PREAMBLE, __func__, *write_protect);

	return err;
}

static esp_err_t spi_flash_chip_zetta_set_write_protect(esp_flash_t *chip, bool write_protect)
{
	static const char *register_name = "Block lock";
	uint8_t register_value = 0;
	esp_err_t err = ESP_OK;

	// Read the current register value
	err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressBlockLock, &register_value);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s: failed to perform GET_FEATURES", LOG_PREAMBLE, __func__);

		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s: register %s=%d", LOG_PREAMBLE, __func__, register_name, register_value);

	// Prepare the register value
	if (write_protect) {
		// Enable write protection for all panes
		register_value |= get_zd35_full_lock_mask();  // The mask coincides w/ required bit values
	} else {
		// Disable write protection for all panes
		register_value &= ~get_zd35_full_lock_mask();  // reset irrelevant registers
		register_value |= (Zd35RegisterBlockLockBrwdMask | Zd35registerBlockLockTb);
	}

	// Write the register value
	{
		ESP_LOGV(DEBUG_TAG, "%s:%s: Writing register %s=%d", LOG_PREAMBLE, __func__, register_name, register_value);
		err = spi_flash_chip_zetta_perform_set_features(chip, Zd35AddressBlockLock, register_value);

		if (err != ESP_OK) {
			ESP_LOGE(DEBUG_TAG, "%s:%s failed to set/reset write protection", LOG_PREAMBLE, __func__);

			return err;
		}
	}

#if 1  // Additional check for debugging purposes
	err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressBlockLock, &register_value);

	if (err != ESP_OK) {
		ESP_LOGI(DEBUG_TAG, "%s:%s: failed to perform GET_FEATURES", LOG_PREAMBLE, __func__);

		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s: register after writing %s=%d", LOG_PREAMBLE, __func__, register_name, register_value);
#endif

	return err;
}

esp_err_t spi_flash_chip_issi_set_io_mode(esp_flash_t *chip);
esp_err_t spi_flash_chip_issi_get_io_mode(esp_flash_t *chip, esp_flash_io_mode_t* out_io_mode);

// Use the same implementation as ISSI chips
#define spi_flash_chip_zetta_set_io_mode spi_flash_chip_issi_set_io_mode
#define spi_flash_chip_zetta_get_io_mode spi_flash_chip_issi_get_io_mode
#define spi_flash_chip_zetta_read_reg spi_flash_chip_generic_read_reg

static const char chip_name[] = "zetta";

static spi_flash_caps_t spi_flash_chip_zetta_get_caps(esp_flash_t *chip)
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

	.get_chip_write_protect = spi_flash_chip_zetta_get_write_protect,
	.set_chip_write_protect = spi_flash_chip_zetta_set_write_protect,

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
