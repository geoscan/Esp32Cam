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

/// \brief Performs "READ FROM CACHE" operation which is usually preceded by
/// "PAGE READ" which stores a selected page into the memory chip's fast random
/// access cache.
///
/// \param `cache_offset` is an offset position from which the cache will be
/// read.
/// \pre `cache_offset` Must be in a SPI-compatible format. For example, some
/// Zetta chips stipulate the use of a "pane select" bit when accessing the
/// cache.
static inline esp_err_t spi_flash_chip_zetta_perform_read_from_cache(esp_flash_t *chip, void *buffer, uint32_t cache_offset,
	uint32_t length);

/// \brief Will produce a SPI-compatible address accouting for pane selection
/// bit for 2 GB versions.
///
/// \note The driver may not support some versions of Zetta SPI Flash memory
/// yet, which the user will be notified of through return value.
static esp_err_t spi_flash_chip_zetta_address_to_spi_page_address(esp_flash_t *chip, uint32_t absolute_address,
	uint32_t *out_spi_page_address);

/// \brief Performs "PAGE READ" operation which puts the selected page into the
/// SPI device's fast random access cache for later access.
/// \param `page_address` specifies offset of that page
/// \pre `page_address` will have had to be subjected to boundary checks by the
/// time the call is made, as this function performs no such checks.
static esp_err_t spi_flash_chip_zetta_perform_page_read(esp_flash_t *chip, uint32_t page_address);

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

static esp_err_t spi_flash_chip_zetta_address_to_spi_page_address(esp_flash_t *chip, uint32_t absolute_address,
	uint32_t *out_spi_page_address)
{
	switch (chip->chip_id) {
		case Zd35x2ChipId: {
			if (absolute_address > Zd35x2CapacityBytes) {
				return ESP_ERR_INVALID_ARG;
			}

			*out_spi_page_address = absolute_address / chip->chip_drv->page_size;

			return ESP_OK;
		}

		default:
			return ESP_ERR_NOT_SUPPORTED;
	}
}

/// Converts the absolute address into 12-bit address compatible with "READ
/// FROM CACHE" operation.
static esp_err_t spi_flash_chip_zetta_address_to_cache_offset(esp_flash_t *chip, uint32_t absolute_address,
	uint32_t *out_cache_offset)
{
	switch (chip->chip_id)  {
		case Zd35x2ChipId: {
			static const int PLANE_SELECT_BYTE_OFFSET = 12;
			uint32_t plane_select_mask = 0;

			// Form SPI-compatible address
			plane_select_mask = (absolute_address / Zd35x2PlaneSizeBytes) << PLANE_SELECT_BYTE_OFFSET;  // See the datasheet, "READ FROM CACHE x1"
			*out_cache_offset = absolute_address % Zd35x2PageSize;
			*out_cache_offset |= plane_select_mask;

			return ESP_OK;
		}

		default:
			return ESP_ERR_NOT_SUPPORTED;
	}
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

static esp_err_t spi_flash_chip_zetta_perform_page_read(esp_flash_t *chip, uint32_t page_address)
{
	static const size_t N_ATTEMPTS = 10;

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
	ESP_LOGV(DEBUG_TAG, "%s:%s: issuing PAGE READ command", LOG_PREAMBLE, __func__);
	esp_err_t err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	if (err != ESP_OK) {
		return err;
	}


	// Poll the device until the page is read
	for (int i = 1; i < N_ATTEMPTS + 1; --i) {
		ESP_LOGV(DEBUG_TAG, "%s:%s: waiting for the status update, attempt #=%d", LOG_PREAMBLE, __func__, i);
		uint8_t register_value = 0;
		err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressStatus, &register_value);

		if (err != ESP_OK) {
			return err;
		}

		if (!(register_value & Zd35RegisterStatusOip)) {
			ESP_LOGV(DEBUG_TAG, "%s:%s: got status update, PAGE READ finished", LOG_PREAMBLE, __func__);
			break;
		} else {
			// Busy wait
			// TODO: yield to another task using FreeRTOS calls. The call below
			// invokes `esp_rom_delay_us` which seems to be a busy-wait or CPU
			// suspend, and has nothing to do with OS's "delay and yield to
			// another task" type of calls
			// chip->os_func->delay_us(chip->os_func_data, 1);
		}
	}

	return err;
}

static inline esp_err_t spi_flash_chip_zetta_perform_read_from_cache(esp_flash_t *chip, void *buffer, uint32_t cache_offset,
	uint32_t length)
{
	if (cache_offset >= Zd35x2CacheSizeBytes || length > Zd35x2CacheSizeBytes
			|| cache_offset + length > Zd35x2CacheSizeBytes) {
		return ESP_ERR_INVALID_ARG;
	}

	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,
		.miso_len = length,
		.address_bitlen = 15,  // 3 dummy bits + 1 plane select bit + 11 address bits  XXX: isn't it meant to be handled by `dummy_bits'?
		.address = cache_offset,
		.mosi_data = NULL,
		.miso_data = buffer,
		.flags = 0,
		.command = Zd35CommandReadFromCache,
		.dummy_bitlen = 8,  // 1 dummy byte
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

static esp_err_t spi_flash_chip_zetta_read(esp_flash_t *chip, void *buffer, uint32_t address, uint32_t length)
{
	// Shadow the content of a NAND array into register, datasheet, p. 29
	esp_err_t err = ESP_OK;
	uint32_t page_address = 0;
	uint32_t cache_offset = 0;  // Offset in the ZD35's cache

	// Read the specified page into cache
	err = spi_flash_chip_zetta_address_to_spi_page_address(chip, address, &page_address);

	if (err != ESP_OK) {
		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s trying to cache page by id=%d", LOG_PREAMBLE, __func__, page_address);
	err = spi_flash_chip_zetta_perform_page_read(chip, page_address);

	if (err != ESP_OK) {
		return err;
	}

	// Read the cached page from the register accounting for the offset
	cache_offset = spi_flash_chip_zetta_address_to_cache_offset(chip, address, &cache_offset);

	if (err != ESP_OK) {
		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s reading the cached page at offset=%d, length=%d", LOG_PREAMBLE, __func__, cache_offset,
		length);
	err = spi_flash_chip_zetta_perform_read_from_cache(chip, buffer, cache_offset, length);

	return err;
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
	.sector_size = Zd35x2BlockSizeBytes,
	.block_erase_size = Zd35x2BlockSizeBytes,

	.get_chip_write_protect = spi_flash_chip_zetta_get_write_protect,
	.set_chip_write_protect = spi_flash_chip_zetta_set_write_protect,

	.num_protectable_regions = 0,
	.protectable_regions = NULL,
	.get_protected_regions = NULL,
	.set_protected_regions = NULL,

	.read = spi_flash_chip_zetta_read,
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
