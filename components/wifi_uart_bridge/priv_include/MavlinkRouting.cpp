//
// MavlinkRouting.cpp
//
// Created on: Aug 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)

#include "wifi_uart_bridge/Receiver.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
#include "socket/Api.hpp"
#include "sub/Rout.hpp"
#include "MavlinkRouting.hpp"
#include <sdkconfig.h>
#include <utility>

namespace Bdg {

using StaticRule = std::tuple<Bdg::EndpointVariant, Bdg::EndpointVariant, Bdg::EndpointVariant>;

template <class F, class T, class R>
static StaticRule makeStaticRule(F &&aFrom, T &&aTo, R &&aReduce)
{
	return StaticRule{Bdg::EndpointVariant(std::forward<F>(aFrom)), Bdg::EndpointVariant(std::forward<T>(aTo)),
		Bdg::EndpointVariant(std::forward<R>(aReduce))};
}

static const StaticRule kStaticRoutingRules[] = {
#if CONFIG_WIFI_UART_BRIDGE_UART_MAVLINK_PROCESS
	makeStaticRule(Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()}, Bdg::NamedEndpoint::Mavlink,
		Bdg::NamedEndpoint::UartMavlinkForwarded),
	makeStaticRule(Bdg::NamedEndpoint::UartMavlinkForwarded, Bdg::UartEndpoint{MavlinkRouting::getMavlinkUdpPort()},
		Bdg::NamedEndpoint::None),
#else
	makeStaticRule(Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()},
		Bdg::UdpPort{MavlinkRouting::getMavlinkUdpPort()}, Bdg::NamedEndpoint::None),
#endif
#if CONFIG_WIFI_UART_BRIDGE_UDP_MAVLINK_PROCESS
	makeStaticRule(Bdg::UdpPort{MavlinkRouting::getMavlinkUdpPort()}, Bdg::NamedEndpoint::Mavlink,
		Bdg::NamedEndpoint::UdpMavlinkForwarded),
	makeStaticRule(Bdg::NamedEndpoint::UdpMavlinkForwarded, Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()},
		Bdg::NamedEndpoint::None),
#else
	makeStaticRule(Bdg::UdpPort{MavlinkRouting::getMavlinkUdpPort()},
		Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()}, Bdg::NamedEndpoint::None),
#endif
	makeStaticRule(Bdg::NamedEndpoint::MavlinkIpPackForwarded, Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()},
		Bdg::NamedEndpoint::None),
	makeStaticRule(Bdg::NamedEndpoint::Mavlink, Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()},
		Bdg::NamedEndpoint::None),
	makeStaticRule(Bdg::NamedEndpoint::Mavlink, Bdg::UdpPort{MavlinkRouting::getMavlinkUdpPort()},
		Bdg::NamedEndpoint::None),
};

MavlinkRouting::MavlinkRouting()
{
	clientsUdp.reserve(2);
	receivers.reserve(3);
	ESP_LOGI(Bdg::kDebugTag, "MavlinkRouting::CTOR creating UART hook");
	receivers.push_back({{Bdg::UartEndpoint{getMavlinkUartNum()}},  // UART sender
		[](Bdg::OnReceiveCtx aCtx)
		{
			ESP_LOGV(Bdg::kDebugTag, "MavlinkRouting::receivers UART %d bytes", aCtx.buffer.size());
			Sub::Rout::UartSend::notify({Utility::makeAsioCb(aCtx.buffer), getMavlinkUartNum()});
		}});
	ESP_LOGI(Bdg::kDebugTag, "MavlinkRouting::CTOR creating UDP->Mavlink HOOK for port %d", getMavlinkUdpPort());
	receivers.push_back({{Bdg::NamedEndpoint::Mavlink},  // Hook, updates the list of UDP clients
		[this](Bdg::OnReceiveCtx aCtx)
		{
			aCtx.endpointVariant.match(
				[this](const Bdg::UdpEndpoint &aEndpoint)
				{
					if (std::get<1>(aEndpoint) == getMavlinkUdpPort()) {
						const auto endpoint = std::get<0>(aEndpoint);
						auto it = std::find(clientsUdp.begin(), clientsUdp.end(), endpoint);

						if (clientsUdp.end() == it) {
							clientsUdp.push_back(std::get<0>(aEndpoint));
							ESP_LOGI(Bdg::kDebugTag, "MavlinkRouting UDP hook Added a new client %s %d",
								endpoint.address().to_string().c_str(), endpoint.port());
						}
					}
				},
				[](...){});
		}});

	if (Sock::Api::checkInstance()) {
		ESP_LOGI(Bdg::kDebugTag, "MavlinkRouting::CTOR creating Mavlink UDP hook for port %d", getMavlinkUdpPort());
		receivers.push_back({{Bdg::UdpPort{getMavlinkUdpPort()}},
			[this](Bdg::OnReceiveCtx aCtx)  // Iterates over the list of UDP clients, notifies each one of them
			{
				ESP_LOGV(Bdg::kDebugTag, "MavlinkRouting::receivers UDP notifying udp clients from port %d",
					getMavlinkUdpPort());
				for (auto &endpoint : clientsUdp) {
					asio::error_code err;
					auto port = getMavlinkUdpPort();
					Sock::Api::getInstance().sendTo(endpoint, port, Utility::makeAsioCb(aCtx.buffer), err);
				}
			}});
	}

	// Apply routing rules based on the project configuration
	ESP_LOGI(Bdg::kDebugTag, "MavlinkRouting::CTOR initializing static routing rules");
	if (Bdg::RoutingRules::checkInstance()) {
		for (const auto &staticRule : kStaticRoutingRules) {
			Bdg::RoutingRules::getInstance().addStatic(std::get<0>(staticRule), std::get<1>(staticRule),
				std::get<2>(staticRule));
		}
	}
}

MavlinkRouting::~MavlinkRouting()
{
	if (Bdg::RoutingRules::checkInstance()) {
		for (const auto &staticRule : kStaticRoutingRules) {
			Bdg::RoutingRules::getInstance().remove(std::get<0>(staticRule), std::get<1>(staticRule));
		}
	}
}

}  // namespace Bdg
