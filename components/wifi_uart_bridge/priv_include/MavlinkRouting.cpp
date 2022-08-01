//
// MavlinkRouting.cpp
//
// Created on: Aug 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "wifi_uart_bridge/Receiver.hpp"
#include "wifi_uart_bridge/RoutingRules.hpp"
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
#endif
#if CONFIG_WIFI_UART_BRIDGE_UDP_MAVLINK_PROCESS
	makeStaticRule(Bdg::UdpPort{MavlinkRouting::getMavlinkUdpPort()}, Bdg::NamedEndpoint::Mavlink,
		Bdg::NamedEndpoint::UdpMavlinkForwarded),
#else
	makeStaticRule(Bdg::UdpPort{MavlinkRouting::getMavlinkUdpPort()},
		Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()}, Bdg::NamedEndpoint::None),
#endif
	makeStaticRule(Bdg::NamedEndpoint::MavlinkIpPackForwarded, Bdg::UartEndpoint{MavlinkRouting::getMavlinkUartNum()},
		Bdg::NamedEndpoint::None)
};

MavlinkRouting::MavlinkRouting()
{
	clientsUdp.reserve(2);
	receivers.reserve(3);
	receivers.push_back({{Bdg::UartEndpoint{getMavlinkUartNum()}},  // UART sender
		[](Bdg::OnReceiveCtx aCtx)
		{
			Sub::Rout::UartSend::notify({Utility::makeAsioCb(aCtx.buffer), getMavlinkUartNum()});
		}});
	receivers.push_back({{Bdg::NamedEndpoint::Mavlink},  // Hook, updates the list of UDP clients
		[this](Bdg::OnReceiveCtx aCtx)
		{
			aCtx.endpointVariant.match(
				[this](const Bdg::UdpEndpoint &aEndpoint)
				{
					if (std::get<1>(aEndpoint) == getMavlinkUdpPort()) {
						clientsUdp.push_back(std::get<0>(aEndpoint));
					}
				},
				[](...){});
		}});

	if (Sock::Api::checkInstance()) {
		receivers.push_back({{Bdg::UdpPort{getMavlinkUdpPort()}},
			[this](Bdg::OnReceiveCtx aCtx)  // Iterates over the list of UDP clients, notifies each one of them
			{
				for (auto &endpoint : clientsUdp) {
					asio::error_code err;
					auto port = getMavlinkUdpPort();
					Sock::Api::getInstance().sendTo(endpoint, port, Utility::makeAsioCb(aCtx.buffer), err);
				}
			}});
	}

	// Apply routing rules based on the project configuration
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
