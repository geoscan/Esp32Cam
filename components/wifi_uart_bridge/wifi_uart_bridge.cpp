#include <pthread.h>
#include <driver/uart.h>
#include <driver/gpio.h>

#include "wifi_uart_bridge.hpp"
#include "utility/thr/Threading.hpp"
#include "Bridge.hpp"
#include "UartEndpoint.hpp"
#include "UdpEndpoint.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
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

void init()
{
	static Routing routing;
	(void)routing;
}

}  // namespace Bdg
