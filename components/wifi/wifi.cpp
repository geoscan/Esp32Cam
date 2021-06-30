#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "utility/Algorithm.hpp"

#include <string.h>
#include <stdlib.h>

#include "lwip/err.h"
#include "lwip/sys.h"

// https://en.wikipedia.org/wiki/Service_set_(802.11_network)
#define SSID_MAX_LENGTH    32
#define MAC_LENGTH         6

#define min(a,b) (a<b?a:b)

#define CUSTOM_ESP_ERR_RETURN(x)  \
	{	esp_err_t err;  \
		if ((err = (x)) != ESP_OK)   \
			return err;  \
	}


wifi_config_t sStaWifiConfig;
wifi_config_t sApWifiConfig;
esp_netif_t *sStaEspNetif = NULL;

///
/// \brief get_ssid Decorates SSID with MAC address, using the latter as a suffix
/// \param data     Return value
/// \param len      Return value
/// \param prefix   OPTIONAL: if NULL, configuration file will be used
///
static void decorateSsid(uint8_t **data, unsigned *len, const char *prefix)
{
	static uint8_t ssid[SSID_MAX_LENGTH] = {'\0'};
	static uint8_t mac[MAC_LENGTH]       = {'\0'};
	const unsigned prefixlen             = strlen(prefix);

	memset(ssid, 0, sizeof(ssid));
	memset(mac, 0, sizeof(mac));
	memcpy(ssid, prefix, prefixlen);

	esp_efuse_mac_get_default(mac);
	for (int i = 0; i < MAC_LENGTH; ++i) {
		sprintf((char *)ssid + prefixlen + i*2, "%02x", mac[i]);
	}

	*data = ssid;
	*len  = strlen((char *)ssid);
}

///
/// \brief wifiConfigStaConnection configures client (the chip is a station) connection parameters
///
/// \param targetApName     Name of the Wi-Fi network the chip should connect to
/// \param targetApPassword Password to access the Wi-Fi network
/// \param ip               Ip of the client, if NULL, a DHCP client will be used
/// \param gateway          Gateway of the network, if NULL, a DHCP client will be used
/// \param netmask          Netmask of the network, if NULL, a DHCP client will be used
///
esp_err_t wifiConfigStaConnection(const char *targetApSsid, const char *targetApPassword, uint8_t ip[4], uint8_t gateway[4], uint8_t netmask[4])
{
	const bool useDhcp = (ip == NULL || gateway == NULL || netmask == NULL);

	// If the Wi-Fi has already been initialized, we will try to connect later
	const esp_err_t disconnectResult = esp_wifi_disconnect();
	const bool shouldConnect = !(disconnectResult == ESP_ERR_WIFI_NOT_INIT || disconnectResult == ESP_ERR_WIFI_NOT_STARTED);

	if (sStaEspNetif == NULL) {
		sStaEspNetif = esp_netif_create_default_wifi_sta();
	}

	if (useDhcp) {
		esp_err_t err = esp_netif_dhcpc_start(sStaEspNetif);
		if (!Utility::Algorithm::in(err, ESP_OK, ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)) {
			return err;
		}
	} else {
		{
			esp_err_t err = esp_netif_dhcpc_stop(sStaEspNetif);
			if (!Utility::Algorithm::in(err, ESP_OK, ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
				return err;
			}
		}

		esp_netif_ip_info_t netifIpInfo;
		IP4_ADDR(&netifIpInfo.ip, ip[0], ip[1], ip[2], ip[3]);
		IP4_ADDR(&netifIpInfo.gw, gateway[0], gateway[1], gateway[2], gateway[3]);
		IP4_ADDR(&netifIpInfo.netmask, netmask[0], netmask[1], netmask[2], netmask[3]);

		CUSTOM_ESP_ERR_RETURN(esp_netif_set_ip_info(sStaEspNetif, &netifIpInfo));
	}

	memset(&sStaWifiConfig, 0, sizeof(sStaWifiConfig));
	sStaWifiConfig.sta.pmf_cfg.capable = true;
	sStaWifiConfig.sta.pmf_cfg.required = false;
	strcpy((char *)sStaWifiConfig.sta.password, targetApPassword);
	strcpy((char *)sStaWifiConfig.sta.ssid, targetApSsid);

	CUSTOM_ESP_ERR_RETURN(esp_wifi_set_config(ESP_IF_WIFI_STA, &sStaWifiConfig));

	if (shouldConnect) {
		return esp_wifi_connect();
	}

	return ESP_OK;
}

///
/// \brief wifiConfigApConnection Sets and applies AP-related configs
/// \param aMaxClients
/// \param aSsid
/// \param aPassword
///
static void wifiConfigApConnection(const uint8_t aMaxClients, const char *aSsid, const char *aPassword)
{
	esp_netif_create_default_wifi_ap();

	const bool usePassword = !(aPassword == NULL || strlen(aPassword) == 0);

	memset(&sApWifiConfig, 0, sizeof(sApWifiConfig));
	sApWifiConfig.ap.ssid_len = 0;  // auto
	sApWifiConfig.ap.ssid_hidden = 0;  // visible SSID
	sApWifiConfig.ap.beacon_interval = 100;  // default
	sApWifiConfig.ap.channel = 0;  // auto
	sApWifiConfig.ap.max_connection = aMaxClients;
	sApWifiConfig.ap.authmode = (usePassword ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN);

	strcpy((char *)&sApWifiConfig.ap.ssid, (char *)aSsid);
	strcpy((char *)&sApWifiConfig.ap.password, aPassword);

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &sApWifiConfig) );
}

static void wifiDriverInit()
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;
	cfg.ampdu_rx_enable = 0;
	cfg.ampdu_tx_enable = 0;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void wifi_init_sta(void)
{
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	ESP_ERROR_CHECK(esp_netif_init());
	wifiDriverInit();
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );

	wifiConfigStaConnection("", "", NULL, NULL, NULL);  // Trigger the initialization process

	uint8_t  *ssid;
	unsigned ssid_len;
	decorateSsid(&ssid, &ssid_len, CONFIG_ESP_WIFI_SSID);
	wifiConfigApConnection(CONFIG_ESP_MAX_STA_CONN, (char *)ssid, CONFIG_ESP_WIFI_PASSWORD);

	ESP_ERROR_CHECK(esp_wifi_start() );
}

void wifiStart(void)
{
	wifi_init_sta();
}
