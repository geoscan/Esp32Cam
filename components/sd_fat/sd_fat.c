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
#include <diskio_impl.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#ifndef FF_DRV_NOT_USED
# define FF_DRV_NOT_USED 0xFF
#endif

static const int kSlotId = 1;
static sdmmc_card_t *cardConfig;
static BYTE pdrv = FF_DRV_NOT_USED;  // Not Used
static bool initialized = false;
static const char *kTag = "[sd_fat]";

static esp_err_t initializeSlot()
{
	static const sdmmc_slot_config_t slotConfig = {
		.gpio_cd = -1,  // No card detect (CD)
		.gpio_wp = -1,  // No write protect (WP)
		.width   =  0,
		.flags   =  0,
	};

	return sdmmc_host_init_slot(kSlotId, &slotConfig);
}

static esp_err_t initializeCard()
{
	static sdmmc_host_t hostConfig = SDMMC_HOST_DEFAULT();
	cardConfig = malloc(sizeof(sdmmc_card_t));

	return sdmmc_card_init(&hostConfig, cardConfig);
}

static esp_err_t mountFat()
{
	enum {
		MountOnFirstAccess = 0,
		MountRightNow = 1
	};

	FATFS *fs = NULL;
	char fatDrivePath[] = {'\0', ':', '\0'};
	esp_err_t err;

	err = ff_diskio_get_drive(&pdrv);  // connect SDMMC driver to FATFS
	if (err != ESP_OK && err == FF_DRV_NOT_USED) {
		return ESP_ERR_NO_MEM;
	}
	fatDrivePath[0] = (char)('0' + pdrv);

	ff_diskio_register_sdmmc(pdrv, cardConfig);  // Register driver

	err = esp_vfs_fat_register(CONFIG_SD_FAT_MOUNT_POINT, fatDrivePath, CONFIG_SD_FAT_MAX_OPENED_FILES, &fs);  // Connect FAT FS to VFS
	if (err != ESP_ERR_INVALID_STATE && err != ESP_OK) {
		return err;
	}

	err = f_mount(fs, fatDrivePath, MountRightNow) == FR_OK ? ESP_OK : ESP_FAIL;

	return err;
}

// Initializes SD card, it relies on presence of FAT-family filesystem on the
// card.

bool sdFatInit()
{
	ESP_LOGI(kTag, "initializing SD card");

	if (initialized) {
		ESP_LOGI(kTag, "initializing SD card -- success (already initialized)");
		return true;
	}

	esp_err_t err = ESP_OK;

	err = sdmmc_host_init();

	if (err == ESP_OK) {
		err = initializeSlot();
	}

	if (err == ESP_OK) {
		err = initializeCard();
	}

	if (err == ESP_OK) {
		err = mountFat();
	}

	initialized = (err == ESP_OK);
	if (initialized) {
		ESP_LOGI(kTag, "initializing SD card -- success");
	} else {
		ESP_LOGE(kTag, "initalizing SD card -- FAILED");
	}

	return initialized;
}

bool sdFatDeinit()
{
	ESP_LOGI(kTag, "deinitializing SD card");

	if (!initialized) {
		ESP_LOGI(kTag, "deinitializing SD card -- success (already deinitialized)");
		return true;
	}

	initialized = !(sdmmc_host_deinit() == ESP_OK);

	if (!initialized) {
		ESP_LOGI(kTag, "deinitializing SD card -- success");
	} else {
		ESP_LOGE(kTag, "deinitalizing SD card -- FAILED");
	}

	return !initialized;
}

void sdFatWriteTest()
{
	FILE *f = fopen(CONFIG_SD_FAT_MOUNT_POINT"/test.txt", "w");
	if (f == NULL) {
		return;
	}

	fprintf(f, "Geoscan, Geoscan, Geoscan. Geoscan!");
	fclose(f);
}
