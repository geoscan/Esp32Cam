#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <string.h>
#include <stdlib.h>

#include "lwip/err.h"
#include "lwip/sys.h"

// https://en.wikipedia.org/wiki/Service_set_(802.11_network)
#define SSID_MAX_LENGTH    32
#define MAC_LENGTH         6

#define min(a,b) (a<b?a:b)

extern void getApNameSuffix(uint8_t **data, unsigned *len);

static void get_ssid(uint8_t **data, unsigned *len)
{
	static uint8_t ssid[SSID_MAX_LENGTH] = {'\0'};
	static uint8_t mac[MAC_LENGTH]       = {'\0'};
	const unsigned prefixlen             = strlen(CONFIG_ESP_WIFI_SSID);

	memcpy(ssid, CONFIG_ESP_WIFI_SSID, prefixlen);

	esp_efuse_mac_get_default(mac);
	for (int i = 0; i < MAC_LENGTH; ++i) {
		sprintf((char *)ssid + prefixlen + i*2, "%02x", mac[i]);
	}

	*data = ssid;
	*len  = strlen((char *)ssid);
}

static void wifi_init_softap(void)
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_AP);

	esp_event_loop_create_default();
	esp_netif_init();
	esp_netif_create_default_wifi_ap();

	uint8_t  *ssid;
	unsigned ssid_len;

	get_ssid(&ssid, &ssid_len);

    wifi_config_t wifi_config;
	memset(&wifi_config, 0, sizeof(wifi_config_t));

	wifi_config.ap.ssid_len = 0;  // auto
	wifi_config.ap.ssid_hidden = 0;  // visible SSID
	wifi_config.ap.beacon_interval = 100;  // default
    wifi_config.ap.channel = 0;  // auto
    wifi_config.ap.max_connection = CONFIG_ESP_MAX_STA_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

	strcpy((char *)&wifi_config.ap.ssid, (char *)ssid);
	strcpy((char *)&wifi_config.ap.password, CONFIG_ESP_WIFI_PASSWORD);

	if (strlen(CONFIG_ESP_WIFI_PASSWORD) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
	esp_wifi_start();
}

void wifiStart(void)
{
    //Initialize NVS
//    esp_err_t ret = nvs_flash_init();
//    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//      nvs_flash_erase();
//      ret = nvs_flash_init();
//    }

    wifi_init_softap();
}
