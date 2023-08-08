//
// DisconnectHandler.cpp
//
// Created on: Aug 08, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "wifi.h"
#include "wifi/Sub.hpp"
#include <esp_log.h>
#include <esp_wifi.h>
#include <cstdint>

#include "DisconnectHandler.hpp"

namespace Wifi {

static constexpr const char *kLogPreamble = "Wifi::DisconnectHandler";

DisconnectHandler::DisconnectHandler()
{
	ESP_LOGI(Wifi::kDebugTag, "%s: Registering event handler for event=%s:%d", kLogPreamble, WIFI_EVENT,
		static_cast<int>(WIFI_EVENT_AP_STADISCONNECTED));
	esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED,
		dispatchDisconnectedEvent, nullptr);
}

DisconnectHandler::~DisconnectHandler()
{
	ESP_LOGI(Wifi::kDebugTag, "%s: Unregistering event handler for event=%s:%d", kLogPreamble, WIFI_EVENT,
		static_cast<int>(WIFI_EVENT_AP_STADISCONNECTED));
	esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, dispatchDisconnectedEvent);
}

void DisconnectHandler::onDisconnected(void *aData)
{
	// Check the availability of the context
	if (aData == nullptr) {
		ESP_LOGE(Wifi::kDebugTag, "%s:%s event has been triggered, but no context provided (nullptr). Skipping",
			kLogPreamble, __func__);

		return;
	}

	// Notify subscribers upon the event
	ip4_addr_t ipAddress;

	if (!dhcp_search_ip_on_mac(reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac, &ipAddress)) {
		ESP_LOGE(Wifi::kDebugTag,
			"%s:%s: Unable to restore corresponding IP from MAC %02X%02X%02X%02X%02X%02X. Skipping",
			kLogPreamble, __func__,
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[0],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[1],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[2],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[3],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[4],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[5]);

		return;
	}

	const auto asioIp = asio::ip::address_v4(ntohl(ipAddress.addr));
	ESP_LOGI(Wifi::kDebugTag, "%s:%s: Disconnected IP=%d.%d.%d.%d MAC=%02X%02X%02X%02X%02X%02X", kLogPreamble, __func__,
			reinterpret_cast<std::uint8_t *>(&ipAddress.addr)[0],
			reinterpret_cast<std::uint8_t *>(&ipAddress.addr)[1],
			reinterpret_cast<std::uint8_t *>(&ipAddress.addr)[2],
			reinterpret_cast<std::uint8_t *>(&ipAddress.addr)[3],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[0],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[1],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[2],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[3],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[4],
			reinterpret_cast<system_event_ap_stadisconnected_t *>(aData)->mac[5]);
	Wifi::Sub::Disconnected::notify(asioIp);
}

void DisconnectHandler::dispatchDisconnectedEvent(void */*nullptr*/, esp_event_base_t, int32_t, void *aData)
{
	DisconnectHandler::getInstance().onDisconnected(aData);
}

}  // Wifi
