#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)

#include <pthread.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#include "wifi_uart_bridge.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "wifi_uart_bridge/Receiver.hpp"
#include "MavlinkRouting.hpp"
#include "socket/Api.hpp"
#include <vector>

namespace Bdg {

void init()
{
	esp_log_level_set(Bdg::kDebugTag, LOG_LOCAL_LEVEL);
	ESP_LOGD(Bdg::kDebugTag, "Debug log test");
	ESP_LOGV(Bdg::kDebugTag, "Verbose log test");
	static Bdg::RoutingRules routingRules{};
	static Bdg::MavlinkRouting mavlinkRouting{};
	(void)routingRules;
	(void)mavlinkRouting;
}

}  // namespace Bdg
