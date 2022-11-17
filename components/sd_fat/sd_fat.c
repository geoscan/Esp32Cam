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
static sdmmc_card_t *cardConfig = NULL;
static BYTE pdrv = FF_DRV_NOT_USED;  // Not Used
static const char *kTag = "[sd_fat]";
static FATFS *sFatfs = NULL;
static char fatDrivePath[] = {'\0', ':', '\0'};

/// \brief Boilerplate reducer
static inline void logError(const char *method, const char *context, esp_err_t err)
{
	ESP_LOGE(kTag, "%s (%s) -- error %d (%s)", method, context, err, esp_err_to_name(err));
}

/// \brief Boilerplate reducer
static inline void logWarning(const char *method, const char *context, esp_err_t err)
{
	ESP_LOGW(kTag, "%s (%s) -- error %d (%s)", method, context, err, esp_err_to_name(err));
}

/// \brief SD card peripheral uses the same output pins as JTAG does. This
/// function reconfigures the pins, so they can be used for JTAG debugging. \sa
/// `sdFatDeinit`. Reset pins 12 through 15, so they can be used for JTAG
/// connection.
///
/// \details More info on how "GPIO_MUX" and GPIO Matrix work in this manual:
/// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf#iomuxgpio
static esp_err_t pinsDeinit()
{
	const gpio_num_t kGpioNums[] = {
		GPIO_NUM_12,
		GPIO_NUM_13,
		GPIO_NUM_14,
		GPIO_NUM_15
	};
	// Restores pin default configuration. Refer to
	// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf#iomuxgpio
	// table "4-3" for the complete list of pin default configurations.
	const gpio_config_t kGpioConfigs[] = {
		{
			.pin_bit_mask = BIT64(kGpioNums[0]),
			.mode = GPIO_MODE_INPUT,
			.pull_up_en = false,
			.pull_down_en = true,
			.intr_type = GPIO_INTR_DISABLE
		},
		{
			.pin_bit_mask = BIT64(kGpioNums[1]),
			.mode = GPIO_MODE_INPUT,
			.pull_up_en = false,
			.pull_down_en = true,
			.intr_type = GPIO_INTR_DISABLE
		},
		{
			.pin_bit_mask = BIT64(kGpioNums[2]),
			.mode = GPIO_MODE_INPUT,
			.pull_up_en = true,
			.pull_down_en = false,
			.intr_type = GPIO_INTR_DISABLE
		},
		{
			.pin_bit_mask = BIT64(kGpioNums[3]),
			.mode = GPIO_MODE_INPUT,
			.pull_up_en = true,
			.pull_down_en = false,
			.intr_type = GPIO_INTR_DISABLE
		},
	};
	const int knPins = sizeof(kGpioConfigs) / sizeof(gpio_config_t);
	esp_err_t result = ESP_OK;

	for (int i = 0; i < knPins; ++i) {
		const int kFunctionIdMt = 0;  // The configured pins provide "MTDI", "MTCK", "MTMS", and "MTDO" as alternative functions. Coincidentally, each one of these functions has index "0"
		const bool fInvert = false;
		esp_err_t err = gpio_config(&kGpioConfigs[i]);

		if (err != ESP_OK) {
			ESP_LOGE(kTag, "pinsDeinit -- error, pin #%d, error %d %s", kGpioNums[i], err, esp_err_to_name(err));
		}

		if (result == ESP_OK) {
			result = err;
		}

		gpio_iomux_out(kGpioNums[i], kFunctionIdMt, fInvert);
	}

	return result;
}

/// \brief Hardware initialization
static esp_err_t initializeSlot()
{
	esp_err_t err = ESP_OK;

	{
		static const sdmmc_slot_config_t slotConfig = {
			.gpio_cd = -1,  // No card detect (CD)
			.gpio_wp = -1,  // No write protect (WP)
			.width   =  0,
			.flags   =  0,
		};
		err = sdmmc_host_init_slot(kSlotId, &slotConfig);

		if (err != ESP_OK) {
			logError("initializeSlot", "init SDMMC host", err);

			return err;
		}
	}

	return ESP_OK;
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

	esp_err_t err = ff_diskio_get_drive(&pdrv);  // connect SDMMC driver to FATFS

	if (err != ESP_OK) {
		if (err == ESP_ERR_INVALID_STATE) {
			logWarning("mountFat", "diskio get drive", err);
			err = ESP_OK;
		} else {
			logError("mountFat", "diskio get drive", err);

			return err;
		}
	}

	fatDrivePath[0] = (char)('0' + pdrv);
	ff_diskio_register_sdmmc(pdrv, cardConfig);  // Register driver
	err = esp_vfs_fat_register(CONFIG_SD_FAT_MOUNT_POINT, fatDrivePath, CONFIG_SD_FAT_MAX_OPENED_FILES, &sFatfs);  // Connect FAT FS to VFS

	if (err != ESP_OK) {
		if (err == ESP_ERR_INVALID_STATE) {
			logWarning("mountFat", "register VFS FAT", err);
			err = ESP_OK;
		} else {
			logError("mountFat", "register VFS FAT", err);

			return err;
		}
	}

	err = f_mount(sFatfs, fatDrivePath, MountRightNow);

	if (err != ESP_OK) {
		logError("mountFat", "f_mount", err);

		return err;
	}

	return ESP_OK;
}

/// \brief Implements SD deinitialization sequence, as described here:
/// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/fatfs.html
static esp_err_t unmountFat()
{
	// Arguments to `f_mount`
	enum {
		MountOnFirstAccess = 0,
		MountRightNow = 1
	};

	FATFS *fatfs = NULL;
	esp_err_t err = f_mount(fatfs, fatDrivePath, MountRightNow);

	if (err != ESP_OK) {
		logError("unmountFat", "f_mount", err);
	}

	if (err == ESP_OK) {
		sdmmc_card_t *sdmmcCard = NULL;
		ff_diskio_register_sdmmc(pdrv, sdmmcCard);
	}

	if (err == ESP_OK) {
		err = esp_vfs_fat_unregister_path(fatDrivePath);
		free(sFatfs);

		if (err != ESP_OK) {
			logError("unmountFat", "unregister path", err);
		}
	}

	return err;
}

// Initializes SD card, it relies on presence of FAT-family filesystem on the
// card.

bool sdFatInit()
{
	esp_err_t err = ESP_OK;
	err = sdmmc_host_init();

	if (err != ESP_OK) {
		logError("sdFatInit", "init host", err);

		return err;
	}

	err = initializeSlot();

	if (err != ESP_OK) {
		logError("sdFatInit", "init slot", err);

		return err;
	}

	err = initializeCard();

	if (err != ESP_OK) {
		logError("sdFatInit", "init card", err);

		return err;
	}

	err = mountFat();

	if (err != ESP_OK) {
		logError("sdFatInit", "mount FAT", err);

		return err;
	}

	ESP_LOGI(kTag, "initializing SD card -- success");

	return (err == ESP_OK);
}

bool sdFatDeinit()
{
	esp_err_t err = ESP_OK;

	{
		esp_err_t errUnmount = unmountFat();

		if (err == ESP_OK) {
			err = errUnmount;
		}

		if (errUnmount != ESP_OK) {
			logError("sdFatDeinit", "unmount FAT", err);
		}
	}
	{
		esp_err_t errHostDeinit = sdmmc_host_deinit();

		if (err == ESP_OK) {
			err = errHostDeinit;
		}

		if (errHostDeinit != ESP_OK) {
			logError("sdFatDeinit", "host deinit", err);
		}
	}
	{
		esp_err_t errPinsDeinit = pinsDeinit();

		if (err == ESP_OK) {
			err = errPinsDeinit;
		}

		if (errPinsDeinit != ESP_OK) {
			logError("sdFatDeinit", "pins deinit", err);
		}
	}

	if (err == ESP_OK) {
		ESP_LOGI(kTag, "sdFatDeinit -- success");
	} else {
		logError("sdFatDeinit", "", err);
	}

	return (err == ESP_OK);
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
