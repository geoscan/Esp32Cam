#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)

#include <pthread.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#include "wifi_uart_bridge.hpp"
#include "utility/thr/Threading.hpp"
#include "Bridge.hpp"
#include "UartEndpoint.hpp"
#include "UdpEndpoint.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "wifi_uart_bridge/Receiver.hpp"
#include "socket/Api.hpp"
#include "Routing.hpp"

void wifiUartBridgeStart(asio::io_context &context)
{

	static UartEndpoint uart(UART_NUM_0, GPIO_NUM_3, GPIO_NUM_1, CONFIG_WIFI_UART_BRIDGE_BAUDRATE,
		UART_PARITY_DISABLE, UART_STOP_BITS_1);
	static UdpEndpoint  udp(context);
	static Bridge       bridge(uart, udp);

	Utility::Thr::threadRun(bridge);
}

namespace Bdg {

static void routingRulesInit(Bdg::RoutingRules &aRoutingRules)
{
	static const Bdg::UartEndpoint kUartMavlink{0};
	static const Bdg::UdpPort kUdpMavlink{8001};
	static Bdg::LambdaReceiver mavlinkUdpClientsRegisterer{{Bdg::NamedEndpoint::Mavlink},
		[](Bdg::OnReceiveCtx aCtx)
		{
			Bdg::RoutingRules::getInstance().addStatic({Bdg::NamedEndpoint::UartMavlinkForwarded},
				aCtx.endpointVariant, {Bdg::NamedEndpoint::None});
		} };  ///< Enqueues MAVLink UDP clients
	(void)mavlinkUdpClientsRegisterer;

#if CONFIG_WIFI_UART_BRIDGE_UART_MAVLINK_PROCESS
	Bdg::RoutingRules::getInstance().addStatic({kUartMavlink}, {Bdg::NamedEndpoint::Mavlink},
		{Bdg::NamedEndpoint::UartMavlinkForwarded});
#endif

#if CONFIG_WIFI_UART_BRIDGE_UDP_MAVLINK_PROCESS
	Bdg::RoutingRules::getInstance().addStatic({kUdpMavlink}, {Bdg::NamedEndpoint::Mavlink},
		{Bdg::NamedEndpoint::UdpMavlinkForwarded});
#else
	Bdg::RoutingRules::getInstance().addStatic({kUdpMavlink}, {kUartMavlink}, {Bdg::NamedEndpoint::None});
#endif
	Bdg::RoutingRules::getInstance().addStatic({Bdg::NamedEndpoint::MavlinkIpPackForwarded}, {kUartMavlink},
		{Bdg::NamedEndpoint::None});
}

void init()
{
	esp_log_level_set(Bdg::kDebugTag, LOG_LOCAL_LEVEL);
	static Routing routing;
	(void)routing;
	static Bdg::RoutingRules routingRules{};
	routingRulesInit(routingRules);
}

}  // namespace Bdg
