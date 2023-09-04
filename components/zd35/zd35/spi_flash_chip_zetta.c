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
static const int STATUS_REGISTER_OIP_POLL_ATTEMPTS = 8;
static const int STATUS_REGISTER_OIP_POLL_WAIT_DURATION_MS = 1;

/// \brief Mask for "Block lock" register unlocking all blocks for writing
/// \details All blocks are unlocked, BRWD value is ignored
static const uint8_t BLOCK_LOCK_UNLOCK_WRITE_ALL_BLOCKS = Zd35registerBlockLockTb;

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
static esp_err_t spi_flash_chip_zetta_program_page(esp_flash_t *chip, const void *buffer, uint32_t address,
	uint32_t length);
static esp_err_t spi_flash_chip_zetta_perform_program_execute(esp_flash_t *chip, uint32_t address);
static esp_err_t spi_flash_chip_zetta_wait_idle(esp_flash_t *chip, uint32_t timeout_us);
static esp_err_t spi_flash_chip_zetta_write(esp_flash_t *chip, const void *buffer, uint32_t address, uint32_t length);
static esp_err_t spi_flash_chip_zetta_config_host_io_mode(esp_flash_t *chip, uint32_t flags);

/// \brief Performs "READ FROM CACHE" operation which is usually preceded by
/// "PAGE READ" which stores a selected page into the memory chip's fast random
/// access cache.
///
/// \param `cache_offset` is an offset position from which the cache will be
/// read.
/// \pre `cache_offset` Must be in a SPI-compatible format. For example, some
/// Zetta chips stipulate the use of a "pane select" bit when accessing the
/// cache.
static inline esp_err_t spi_flash_chip_zetta_perform_read_from_cache(esp_flash_t *chip, void *buffer,
	uint32_t cache_offset, uint32_t length);

/// \brief Will produce a SPI-compatible address accouting for pane selection
/// bit for 2 GB versions.
///
/// \note The driver may not support some versions of Zetta SPI Flash memory
/// yet, which the user will be notified of through return value.
static esp_err_t spi_flash_chip_zetta_address_to_spi_page_address(esp_flash_t *chip, uint32_t absolute_address,
	uint32_t *out_spi_page_address);

/// \brief Will check whether the address has the correct alignment regarding
/// the chip's geometry, and put the resulting erase block offset into
/// `out_relative_address`
static esp_err_t spi_flash_chip_zetta_address_to_block_offset(esp_flash_t *chip, uint32_t absolute_address,
	uint32_t *out_relative_address);

/// \brief Performs "PAGE READ" operation which puts the selected page into the
/// SPI device's fast random access cache for later access.
/// \param `page_address` specifies offset of that page
/// \pre `page_address` will have had to be subjected to boundary checks by the
/// time the call is made, as this function performs no such checks.
static esp_err_t spi_flash_chip_zetta_perform_page_read(esp_flash_t *chip, uint32_t page_address);

/// \brief Performs erase operation on a block (for ZD35X2 it has the length of
/// 64 pages).
/// \note Since ZD35X2 makes no distinction b/w sectors and blocks, the same
/// function is used for sector erase.
static esp_err_t spi_flash_chip_zetta_erase_block(esp_flash_t *chip, uint32_t start_address);

/// \brief Busy-poll "OIP" bit of "Control" register until the operation is
/// finished.
static esp_err_t spi_flash_chip_zetta_poll_wait_oip(esp_flash_t *chip);

static esp_err_t spi_flash_chip_zetta_poll_wait_oip(esp_flash_t *chip)
{
	esp_err_t err = ESP_OK;

	for (int i = 1; i < STATUS_REGISTER_OIP_POLL_ATTEMPTS + 1; --i) {
		ESP_LOGV(DEBUG_TAG, "%s:%s: waiting for the status update, attempt #=%d", LOG_PREAMBLE, __func__, i);
		uint8_t register_value = 0;
		err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressStatus, &register_value);

		if (err != ESP_OK) {
			ESP_LOGE(DEBUG_TAG, "%s:%s Failed to perform \"GET FEATURES\"", LOG_PREAMBLE, __func__);
			return err;
		}

		if (!(register_value & Zd35RegisterStatusOip)) {
			ESP_LOGV(DEBUG_TAG, "%s:%s: got status update, the operation has finished", LOG_PREAMBLE, __func__);

			return ESP_OK;
		} else {
			chip->os_func->delay_us(chip->os_func_data, STATUS_REGISTER_OIP_POLL_WAIT_DURATION_MS);
		}
	}

	return ESP_ERR_TIMEOUT;
}

static esp_err_t spi_flash_chip_zetta_perform_set_features(esp_flash_t *chip, uint8_t register_address,
	uint8_t register_value)
{
	const uint8_t mosi_buffer[] = {register_value};
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 1,  // register address + value thereof
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
				ESP_LOGE(DEBUG_TAG,
					"%s:%s failed to convert address to page address, absolute_address=%d exceeds the chip's capacity=%d",
					LOG_PREAMBLE, __func__, absolute_address, Zd35x2CapacityBytes);

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

static inline esp_err_t spi_flash_chip_zetta_perform_get_features(esp_flash_t *chip, uint8_t register_address,
	uint8_t *out_buffer)
{
	// Configure the transaction to be made
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,  // 1 command byte
		.miso_len = 1,  // 1 output byte
		.address_bitlen = 8,
		.address = register_address,
		.mosi_data = NULL,
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
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,
		.miso_len = 0,
		.address_bitlen = 24,
		.address = page_address,
		.mosi_data = NULL,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandPageRead,
		.dummy_bitlen = 0,
		.io_mode = 0,
	};
	ESP_LOGV(DEBUG_TAG, "%s:%s: issuing PAGE READ command", LOG_PREAMBLE, __func__);
	esp_err_t err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	if (err != ESP_OK) {
		return err;
	}

	// Poll the device until the page is read
	err = spi_flash_chip_zetta_poll_wait_oip(chip);

	return err;
}

static esp_err_t spi_flash_chip_zetta_address_to_block_offset(esp_flash_t *chip, uint32_t absolute_address,
	uint32_t *out_relative_address)
{
	switch (chip->chip_id) {
		case Zd35x2ChipId: {
			// The address must, at most, specify the last block's starting position
			if (absolute_address > Zd35x2CapacityBytes - Zd35x2BlockSizeBytes) {
				ESP_LOGE(DEBUG_TAG,
					"%s:%s failed to convert address to block offset, absolute_address=%d exceeds memory capacity",
					LOG_PREAMBLE, __func__, absolute_address);
				return ESP_ERR_INVALID_ARG;
			}

			// The address is not aligned
			if (absolute_address % Zd35x2BlockSizeBytes != 0) {
				ESP_LOGE(DEBUG_TAG,
					"%s:%s failed to convert address to block offset, absolute_address=%d is not aligned to %d",
					LOG_PREAMBLE, __func__, absolute_address, Zd35x2BlockSizeBytes);
				return ESP_ERR_INVALID_ARG;
			}

			*out_relative_address = absolute_address / Zd35x2BlockSizeBytes;

			return ESP_OK;
		}
		default:
			return ESP_ERR_NOT_SUPPORTED;
	}
}

static esp_err_t spi_flash_chip_zetta_erase_block(esp_flash_t *chip, uint32_t start_address)
{
	esp_err_t err = ESP_OK;
	// Disable on-chip write protection mechanism
	spi_flash_chip_zetta_set_write_protect(chip, false);

	// Perform erase command
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,
		.miso_len = 0,
		.address_bitlen = 24,
		.address = 0,
		.mosi_data = NULL,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandBlockErase,
		.dummy_bitlen = 0,
		.io_mode = 0,
	};
	err = spi_flash_chip_zetta_address_to_block_offset(chip, start_address, &spi_flash_trans.address);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s failed to convert absolute address to block offset", LOG_PREAMBLE, __func__);

		return err;
	}

	err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s failed to execute \"BLOCK ERASE\"", LOG_PREAMBLE, __func__);

		return err;
	}

	// Poll `OIP` bit until the operation is completed
	err = spi_flash_chip_zetta_poll_wait_oip(chip);

	return err;
}

static inline esp_err_t spi_flash_chip_zetta_perform_read_from_cache(esp_flash_t *chip, void *buffer, uint32_t cache_offset,
	uint32_t length)
{
	if (cache_offset >= Zd35x2CacheSizeBytes || length > Zd35x2CacheSizeBytes
			|| cache_offset + length > Zd35x2CacheSizeBytes) {
		ESP_LOGE(DEBUG_TAG, "%s:%s failed to read from cache, invalid cache_offset=%d", LOG_PREAMBLE, __func__,
			cache_offset);

		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s reading %d bytes from cache", LOG_PREAMBLE, __func__, length);
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,
		.miso_len = length,
		.address_bitlen = 16,  // 3 dummy bits + 1 plane select bit + 12 address bits  XXX: isn't it meant to be handled by `dummy_bits'?
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
	uint8_t register_value = 0;
	esp_err_t err = ESP_OK;

	if (chip == NULL || write_protect == NULL) {
		ESP_LOGE(DEBUG_TAG, "%s:%s failed to check write protection, wrong arguments", LOG_PREAMBLE, __func__);

		return ESP_ERR_INVALID_ARG;
	}

	// Check "Block lock" register
	err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressBlockLock, &register_value);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to execute \"GET FEATURES\" for \"Block lock\"", LOG_PREAMBLE, __func__);

		return err;
	}

	if (register_value != BLOCK_LOCK_UNLOCK_WRITE_ALL_BLOCKS) {
		*write_protect = true;

		return ESP_OK;
	}

	// Check "Status" register

	err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressStatus, &register_value);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to execute \"GET FEATURES\" for \"Status\"", LOG_PREAMBLE, __func__);
		return err;
	}

	if (register_value & (Zd35RegisterStatusWel)) {  // WEL stands for "write enable"
		*write_protect = false;
	} else {
		*write_protect = true;
	}

	return err;
}

static esp_err_t spi_flash_chip_zetta_set_write_protect(esp_flash_t *chip, bool write_protect)
{
	static const char *register_name = "Block lock";
	uint8_t register_value = 0;
	esp_err_t err = ESP_OK;

	// Unlock all blocks
	// TODO XXX: check the register value first?
	err = spi_flash_chip_zetta_perform_set_features(chip, Zd35AddressBlockLock, BLOCK_LOCK_UNLOCK_WRITE_ALL_BLOCKS);

	// Issue "WRITE ENABLE" or "WRITE DISABLE", already implemented by ESP-IDF, just making a call to the defalut implementation
	spi_flash_chip_generic_set_write_protect(chip, write_protect);

#if 1  // Additional check for debugging purposes
	err = spi_flash_chip_zetta_perform_get_features(chip, Zd35AddressBlockLock, &register_value);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s: TEST failed to perform GET FEATURES for \"Block lock\"", LOG_PREAMBLE, __func__);

		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s: TEST register after writing %s=%d expected value=%d", LOG_PREAMBLE, __func__,
		register_name, register_value, BLOCK_LOCK_UNLOCK_WRITE_ALL_BLOCKS);
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

static esp_err_t spi_flash_chip_zetta_config_host_io_mode(esp_flash_t *chip, uint32_t flags)
{
	esp_flash_io_mode_t read_mode = chip->read_mode;
	uint32_t address_bitlen = 0;
	uint32_t dummy_bitlen = 0;
	uint32_t read_command = 0;
	(void)flags;

	switch (read_mode & 0xFFFF) {
		case SPI_FLASH_FASTRD: {
			address_bitlen = 16;
			dummy_bitlen = 8;
			read_command = Zd35CommandReadFromCache;

			break;
		}

		case SPI_FLASH_SLOWRD:  // XXX Sense the possibility of using other IO modes
			return ESP_ERR_NOT_SUPPORTED;
	}

	return chip->host->driver->configure_host_io_mode(chip->host, read_command, address_bitlen, dummy_bitlen,
		read_mode);
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
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to convert address=%d to page address", LOG_PREAMBLE, __func__, address);
		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s trying to cache page by id=%d", LOG_PREAMBLE, __func__, page_address);
	err = spi_flash_chip_zetta_perform_page_read(chip, page_address);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to perform \"PAGE READ\" for page id=0x%02x", LOG_PREAMBLE, __func__,
			page_address);
		return err;
	}

	// Read the cached page from the register accounting for the offset
	err = spi_flash_chip_zetta_address_to_cache_offset(chip, address, &cache_offset);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to convert address=%d to cache offset", LOG_PREAMBLE, __func__, address);
		return err;
	}

	ESP_LOGV(DEBUG_TAG, "%s:%s reading the cached page at offset=%d, length=%d", LOG_PREAMBLE, __func__, cache_offset,
		length);
	err = spi_flash_chip_generic_read(chip, buffer, cache_offset, length);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to read page cache at offset=%d", LOG_PREAMBLE, __func__, cache_offset);
	}

	return err;
}

static esp_err_t spi_flash_chip_zetta_program_page(esp_flash_t *chip, const void *buffer, uint32_t address,
	uint32_t length)
{
	esp_err_t err = ESP_OK;
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = length,
		.miso_len = 0,
		.address_bitlen = 16,  // 3 dummy bits + 1 plane select bit + 12 address bits  XXX: isn't it meant to be handled by `dummy_bits'?
		.address = 0,
		.mosi_data = buffer,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandProgramLoad,
		.dummy_bitlen = 0,
		.io_mode = 0,
	};

	// Wait for whatever the operation happens to take place to finish
	err = chip->chip_drv->wait_idle(chip, chip->chip_drv->timeout->idle_timeout);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to wait idle", LOG_PREAMBLE, __func__);
		return err;
	}

	// Issue the command

	err = spi_flash_chip_zetta_address_to_cache_offset(chip, address, &spi_flash_trans.address);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to convert address=%d to cache offset", LOG_PREAMBLE, __func__, address);

		return err;
	}

	err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	return err;
}

static esp_err_t spi_flash_chip_zetta_perform_program_execute(esp_flash_t *chip, uint32_t absolute_address)
{
	esp_err_t err = ESP_OK;
	spi_flash_trans_t spi_flash_trans = (spi_flash_trans_t) {
		.mosi_len = 0,  // register address + value thereof
		.miso_len = 0,
		.address_bitlen = 24,
		.address = 0,
		.mosi_data = NULL,
		.miso_data = NULL,
		.flags = 0,
		.command = Zd35CommandProgramExecute,
		.dummy_bitlen = 0,
		.io_mode = 0,
	};

	err = spi_flash_chip_zetta_address_to_spi_page_address(chip, absolute_address, &spi_flash_trans.address);

	if (err != ESP_OK) {
		return err;
	}

	err = chip->host->driver->common_command(chip->host, &spi_flash_trans);

	return err;
}

static esp_err_t spi_flash_chip_zetta_wait_idle(esp_flash_t *chip, uint32_t timeout_us)
{
	(void)timeout_us;

	return spi_flash_chip_zetta_poll_wait_oip(chip);
}

static esp_err_t spi_flash_chip_zetta_write(esp_flash_t *chip, const void *buffer, uint32_t address, uint32_t length)
{
	esp_err_t err = ESP_OK;

	// Disable write protection, store the page in the chip's cache registers. Use the default ESP IDF implementation - it gets it right
	err = spi_flash_chip_generic_write(chip, buffer, address, length);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG,
			"%s:%s Failed to disable write protection and shadow NAND data into the SPI chip's registers",
			LOG_PREAMBLE, __func__);
		return err;
	}

	// Flush the SPI flash chip's buffer into the actual NAND storage.
	err = spi_flash_chip_zetta_perform_program_execute(chip, address);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to perform \"PROGRAM EXECUTE\" for address=%d", LOG_PREAMBLE, __func__,
			address);
		return err;
	}

	// Wait until it's done
	err = spi_flash_chip_zetta_poll_wait_oip(chip);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to wait for \"Status\" register's \"OIP\" bit to be set to 0", LOG_PREAMBLE,
			__func__);
		return err;
	}

	// Ensure write protection is set back
	err = spi_flash_chip_zetta_set_write_protect(chip, true);

	if (err != ESP_OK) {
		ESP_LOGE(DEBUG_TAG, "%s:%s Failed to perform \"WRITE DISABLE\"", LOG_PREAMBLE, __func__);
	}

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
	.erase_chip = spi_flash_chip_generic_erase_chip,  // TODO: check the implementation
	.erase_sector = spi_flash_chip_zetta_erase_block,
	.erase_block = spi_flash_chip_zetta_erase_block,
	.sector_size = Zd35x2BlockSizeBytes,
	.block_erase_size = Zd35x2BlockSizeBytes,

	.get_chip_write_protect = spi_flash_chip_zetta_get_write_protect,
	.set_chip_write_protect = spi_flash_chip_zetta_set_write_protect,

	.num_protectable_regions = 0,
	.protectable_regions = NULL,
	.get_protected_regions = NULL,
	.set_protected_regions = NULL,

	.read = spi_flash_chip_zetta_read,
	.write = spi_flash_chip_zetta_write,
	.program_page = spi_flash_chip_zetta_program_page,
	.page_size = Zd35x2PageSize,
	.write_encrypted = spi_flash_chip_generic_write_encrypted,

	.wait_idle = spi_flash_chip_zetta_wait_idle,
	.set_io_mode = spi_flash_chip_zetta_set_io_mode,
	.get_io_mode = spi_flash_chip_zetta_get_io_mode,

	.read_reg = spi_flash_chip_zetta_read_reg,
	.yield = spi_flash_chip_generic_yield,
	.sus_setup = spi_flash_chip_generic_suspend_cmd_conf,
	.read_unique_id = spi_flash_chip_generic_read_unique_id_none,
	.get_chip_caps = spi_flash_chip_zetta_get_caps,
	.config_host_io_mode = spi_flash_chip_zetta_config_host_io_mode,
};
