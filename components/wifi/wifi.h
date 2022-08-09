#ifndef COMPONENTS_WIFI_WIFI_H
#define COMPONENTS_WIFI_WIFI_H

#include "esp_err.h"

void wifiStart(void);
esp_err_t wifiStaConnect(const char *targetApSsid, const char *targetApPassword, uint8_t ip[4], uint8_t gateway[4], uint8_t netmask[4]);

#ifdef __cplusplus

namespace Wifi {

static constexpr const char *kDebugTag = "[wifi]";

}  // namespace Wifi

#endif  // __cplusplus

#endif // COMPONENTS_WIFI_WIFI_H
