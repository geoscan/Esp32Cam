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

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

// https://en.wikipedia.org/wiki/Service_set_(802.11_network)
#define SSID_MAX_LENGTH    32
#define MAC_LENGTH         6

#define min(a,b) (a<b?a:b)

static const char *TAG = "wifi softAP";

extern void getApNameSuffix(uint8_t **data, unsigned *len);

static void get_ssid(uint8_t **data, unsigned *len)
{
	static uint8_t ssid[SSID_MAX_LENGTH] = {'\0'};
	static uint8_t mac[MAC_LENGTH]       = {'\0'};
	unsigned       ssidlen               = min(SSID_MAX_LENGTH - MAC_LENGTH * 2, strlen(ESP_WIFI_SSID));

	memcpy(ssid, ESP_WIFI_SSID, ssidlen);

	// Copy MAC
	esp_efuse_mac_get_default(mac);
	for (int i = 0; i < MAC_LENGTH; ++i) {
		sprintf((char *)ssid + ssidlen + i*2, "%02x", mac[i]);
//		*(ssid + ssidlen + i) = mac[i];
	}

	*data = ssid;
	*len  = strlen((char *)ssid);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                        ESP_EVENT_ANY_ID,
//                                                        &wifi_event_handler,
//                                                        NULL,
//                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL));

    // Form WIFI AP name (SSID)
//    const unsigned SSID_MAX_LENGTH = 32;
	uint8_t  *ssid;
	unsigned ssid_len;

	get_ssid(&ssid, &ssid_len);

    wifi_config_t wifi_config = {
        .ap = {
//            .ssid = ESP_WIFI_SSID,
//            .ssid_len = strlen(ESP_WIFI_SSID),
			.ssid_len = 0,
			.ssid = {'\0'},
			.ssid_hidden = 0,
			.beacon_interval = 100,
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
	ssid[strlen(ESP_WIFI_SSID) + MAC_LENGTH * 2] = '\0';
    strcpy((char *)wifi_config.ap.ssid, (char *)ssid);
    wifi_config.ap.ssid[strlen(ESP_WIFI_SSID) + MAC_LENGTH * 2] = '\0';

    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
}

void wifiStart(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
}
