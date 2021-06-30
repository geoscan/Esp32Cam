#ifndef COMPONENTS_WIFI_WIFI_H
#define COMPONENTS_WIFI_WIFI_H

#include "esp_err.h"

void wifiStart(void);
esp_err_t wifiConfigStaConnection(const char *targetApSsid, const char *targetApPassword, uint8_t ip[4], uint8_t gateway[4], uint8_t netmask[4]);

#endif // COMPONENTS_WIFI_WIFI_H
