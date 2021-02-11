//
// sd_ftp.c
//
// Created on: Feb 11, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>
#include <esp_vfs_fat.h>
#include <sdkconfig.h>
#include <diskio_sdmmc.h>

static const int kSlotId = 1;
static sdmmc_card_t *cardConfig;

static esp_err_t initializeSlot()
{
	static const sdmmc_slot_config_t slotConfig = {
		.gpio_cd = -1,  // No card detect (CD)
		.gpio_wp = -1,  // No write protect (WP)
		.width   = 0,   // Default width (4, for slot #1)
		.flags   = 0,
	};

	return sdmmc_host_init_slot(kSlotId, &slotConfig);
}

static esp_err_t initializeCard()
{
	static sdmmc_host_t hostConfig = SDMMC_HOST_DEFAULT();
	hostConfig.slot = kSlotId;
	cardConfig = malloc(sizeof(sdmmc_card_t));

	return sdmmc_card_init(&hostConfig, cardConfig);
}

static esp_err_t mountFat()
{
	enum {
		MountOnFirstAccess = 0,
		MountRightNow = 1
	};

	const BYTE kPdrv = 0xFF;  // DRV_NOT_USED
	FATFS *fs = NULL;
	char fatDrivePath[] = {(char)('0' + kPdrv), ':', '\0'};
	esp_err_t err;

	ff_diskio_register_sdmmc(kPdrv, cardConfig);  // Register driver

	err = esp_vfs_fat_register(CONFIG_SD_FAT_MOUNT_POINT, fatDrivePath, CONFIG_SD_FAT_MAX_OPENED_FILES, &fs);  // Connect FAT FS to VFS
	if (err != ESP_ERR_INVALID_STATE && err != ESP_OK) {
		return err;
	}

	err = f_mount(fs, fatDrivePath, MountRightNow) == FR_OK ? ESP_OK : ESP_FAIL;

	return err;
}

void sdFatInit()
{
	if (initializeSlot() != ESP_OK) {
		return;
	}

	if (initializeCard() != ESP_OK) {
		return;
	}

	mountFat();
}