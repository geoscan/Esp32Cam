
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL (CONFIG_WIFI_DEBUG_LEVEL)
#include <esp_log.h>

#include "module/Parameter/Parameter.hpp"
#include "wifi.h"
#include "wifi/Sta.hpp"
#include "wifi/Ap.hpp"
#include "utility/al/Algorithm.hpp"
#include <esp_event.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/err.h>
#include <lwip/sys.h>

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

static constexpr unsigned kBitConnected = BIT0;
static constexpr unsigned kBitDisconnected = BIT1;
static constexpr unsigned kBitStaGotIp = BIT2;

std::string userSsid();

///
/// \brief get_ssid Decorates SSID with MAC address, using the latter as a suffix
/// \param data     Return value
/// \param len      Return value
/// \param prefix   OPTIONAL: if NULL, configuration file will be used
///
static void decorateSsid(char const **data, unsigned *len, const char *prefix)
{
	static char ssid[SSID_MAX_LENGTH] = {'\0'};
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
esp_err_t wifiConfigStaConnection(const char *targetApSsid, const char *targetApPassword, uint8_t ip[4] = nullptr,
	uint8_t gateway[4] = nullptr, uint8_t netmask[4] = nullptr)
{
	const bool useDhcp = (ip == NULL || gateway == NULL || netmask == NULL);

	if (sStaEspNetif == NULL) {
		sStaEspNetif = esp_netif_create_default_wifi_sta();
	}

	// Configure IP acquisition

	if (useDhcp) {
		esp_err_t err = esp_netif_dhcpc_start(sStaEspNetif);
		if (!Ut::Al::in(err, ESP_OK, ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)) {
			ESP_LOGW(Wifi::kDebugTag, "wifiConfigStaConnection() Failed to start DHCP");
			return err;
		}
	} else {
		{
			esp_err_t err = esp_netif_dhcpc_stop(sStaEspNetif);
			if (!Ut::Al::in(err, ESP_OK, ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)) {
				ESP_LOGW(Wifi::kDebugTag, "wifiConfigStaConnection() Failed to stop DHCP");
				return err;
			}
		}

		esp_netif_ip_info_t netifIpInfo;
		IP4_ADDR(&netifIpInfo.ip, ip[0], ip[1], ip[2], ip[3]);
		IP4_ADDR(&netifIpInfo.gw, gateway[0], gateway[1], gateway[2], gateway[3]);
		IP4_ADDR(&netifIpInfo.netmask, netmask[0], netmask[1], netmask[2], netmask[3]);

		CUSTOM_ESP_ERR_RETURN(esp_netif_set_ip_info(sStaEspNetif, &netifIpInfo));
	}

	// Apply config

	memset(&sStaWifiConfig, 0, sizeof(sStaWifiConfig));
	sStaWifiConfig.sta.pmf_cfg.capable = true;
	sStaWifiConfig.sta.pmf_cfg.required = false;
	sStaWifiConfig.sta.channel = 6;
	strcpy((char *)sStaWifiConfig.sta.password, targetApPassword);
	strcpy((char *)sStaWifiConfig.sta.ssid, targetApSsid);

	CUSTOM_ESP_ERR_RETURN(esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &sStaWifiConfig));

	return ESP_OK;
}

static void staHandler(void* arg, esp_event_base_t, int32_t eventId, void* eventData)
{
	EventGroupHandle_t eventGroupHandle = *reinterpret_cast<EventGroupHandle_t *>(arg);

	if (eventId == WIFI_EVENT_STA_CONNECTED) {
		xEventGroupSetBits(eventGroupHandle, kBitConnected);
	} else if (eventId == WIFI_EVENT_STA_DISCONNECTED) {
		xEventGroupSetBits(eventGroupHandle, kBitDisconnected);
	} else if (eventId == IP_EVENT_STA_GOT_IP) {
		xEventGroupSetBits(eventGroupHandle, kBitStaGotIp);
	}
}

///
/// \brief wifiStaConnect Tries to connect to an Access Point. The parameters
/// are the same as for \ref wifiConfigStaConnection
///
esp_err_t wifiStaConnect(const char *targetApSsid, const char *targetApPassword, uint8_t ip[4] = nullptr,
	uint8_t gateway[4] = nullptr, uint8_t netmask[4] = nullptr)
{
	// Disconnect from an AP
	{
		wifi_ap_record_t wifiApRecord{};
		if (ESP_OK == esp_wifi_sta_get_ap_info(&wifiApRecord)) {
			const esp_err_t err = esp_wifi_disconnect();
			if (Ut::Al::in(err, ESP_ERR_WIFI_NOT_INIT, ESP_ERR_WIFI_NOT_STARTED)) {
				ESP_LOGW(Wifi::kDebugTag, "wifiStaConnect() failed to disconnect");
				return err;
			}
		}
	}

	// Update connection configuration
	CUSTOM_ESP_ERR_RETURN(wifiConfigStaConnection(targetApSsid, targetApPassword, ip, gateway, netmask));
	// Establish connection. Wait for the respective flags to be updated
	EventGroupHandle_t wifiEventGroup = xEventGroupCreate();
	esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &staHandler, &wifiEventGroup);
	esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &staHandler, &wifiEventGroup);
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &staHandler, &wifiEventGroup);
	CUSTOM_ESP_ERR_RETURN(esp_wifi_connect());
	const bool useDhcp = Ut::Al::in(nullptr, ip, gateway, netmask);
	EventBits_t eventBits;

	if (useDhcp) {
		constexpr unsigned kDhcpWaitTimeoutMs = 6000 / portTICK_PERIOD_MS;
		eventBits = xEventGroupWaitBits(wifiEventGroup, kBitStaGotIp | kBitDisconnected, pdFALSE, pdFALSE,
			kDhcpWaitTimeoutMs);
	} else {
		eventBits = xEventGroupWaitBits(wifiEventGroup, kBitConnected | kBitDisconnected, pdFALSE, pdFALSE,
			portMAX_DELAY);
	}

	const esp_err_t err = (eventBits & kBitConnected) ? ESP_OK : ESP_FAIL;
	esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &staHandler);
	esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &staHandler);
	esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &staHandler);
	vEventGroupDelete(wifiEventGroup);

	return err;
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
	sApWifiConfig.ap.channel = 1;  // auto
	sApWifiConfig.ap.max_connection = aMaxClients;
	sApWifiConfig.ap.authmode = (usePassword ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN);
	strcpy((char *)&sApWifiConfig.ap.ssid, (char *)aSsid);

	if (usePassword) {
		strcpy((char *)&sApWifiConfig.ap.password, aPassword);
	} else {
		ESP_LOGI(Wifi::kDebugTag, "Creating open Wi-Fi access point");
	}

	ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_AP, &sApWifiConfig) );
}

static void wifiDriverInit()
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;
	cfg.ampdu_rx_enable = 0;
	cfg.ampdu_tx_enable = 0;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

/// \brief Make an attempt to load user SSID from SD card
/// \returns an empty string, if failed
std::string userSsid()
{
	auto *parameter = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiAp, Mod::Fld::Field::Password);
	std::string ret{};

	if (parameter != nullptr) {
		const auto result = parameter->fetch();

		if (result == Mod::Par::Result::Ok) {
			ret = parameter->asStr();
			ESP_LOGI(Wifi::kDebugTag, "Got user SSID: \"%s\"", ret.c_str());
		}
	}

	return ret;
}

/// \brief Make an attempt to load user SSID from SD card
/// \returns True, if succeeded
/// \arg [out] Password
bool tryGetUserPassword(std::string &password)
{
	bool ret = false;
	auto *parameter = Mod::Par::Parameter::instanceByMf(Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier);

	if (parameter != nullptr) {
		const auto result = parameter->fetch();

		if (result == Mod::Par::Result::Ok) {
			password = parameter->asStr();
			ESP_LOGI(Wifi::kDebugTag, "Got user password");
			ret = true;
		}
	}

	return ret;
}

void wifi_init_sta(void)
{
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	ESP_ERROR_CHECK(esp_netif_init());
	wifiDriverInit();
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );
	wifiConfigStaConnection("", "", NULL, NULL, NULL);  // Trigger the initialization process
	auto ussid = userSsid();
	const char *ssid = nullptr;
	std::string userPassword{CONFIG_ESP_WIFI_PASSWORD};

	if (ussid.length() > 0 && ussid.length() <= SSID_MAX_LENGTH) {
		ssid = ussid.c_str();
	} else {
		unsigned ssid_len;
		decorateSsid(&ssid, &ssid_len, CONFIG_ESP_WIFI_SSID);
	}

	wifiConfigApConnection(CONFIG_ESP_MAX_STA_CONN, ssid, userPassword.c_str());
	ESP_ERROR_CHECK(esp_wifi_start() );
}

void wifiStart(void)
{

	esp_log_level_set(Wifi::kDebugTag, static_cast<esp_log_level_t>(LOG_LOCAL_LEVEL));
	ESP_LOGD(Wifi::kDebugTag, "Debug log test");
	ESP_LOGV(Wifi::kDebugTag, "Verbose log test");
	wifi_init_sta();
	static Wifi::Sta sta{&sStaEspNetif};
	static Wifi::Ap ap{};
	(void)sta;
	(void)ap;
}
