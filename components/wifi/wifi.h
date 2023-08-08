#ifndef COMPONENTS_WIFI_WIFI_H
#define COMPONENTS_WIFI_WIFI_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifiStart(void);
esp_err_t wifiStaConnect(const char *targetApSsid, const char *targetApPassword, const uint8_t ip[4],
	const uint8_t gateway[4], const uint8_t netmask[4]);

/// Returns the number of connected stations (Wi-Fi clients)
esp_err_t wifiApCountConnectedStations(unsigned long *aStations);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace Wifi {

static constexpr const char *kDebugTag = "[wifi]";

}  // namespace Wifi

#endif  // __cplusplus

#endif // COMPONENTS_WIFI_WIFI_H
