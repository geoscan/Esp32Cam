//
// Routing.cpp
//
// Created on: Feb 11, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include <sdkconfig.h>
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_WIFI_UART_BRIDGE_DEBUG_LEVEL)
#include <esp_log.h>
#include "utility/LogSection.hpp"

#include "Routing.hpp"
#include "socket/Api.hpp"
#include <sdkconfig.h>
#include "wifi_uart_bridge/wifi_uart_bridge.hpp"

using namespace Bdg;

Sub::Rout::Response Routing::operator()(const Sub::Rout::Uart &aUart)
{
	switch (static_cast<Uart>(aUart.uartNum)) {
		case Uart::Mavlink:
#if CONFIG_WIFI_UART_BRIDGE_UART_MAVLINK_PROCESS
			for (auto &callable : Sub::Rout::OnMavlinkReceived::getIterators()) {
				auto response = callable(aUart.payload);

				if (response.getType() != Sub::Rout::Response::Type::Ignored) {  // Message has been claimed.
					ESP_LOGV(Bdg::kDebugTag, "Routing::operator()(UART) got response; processed %d",
						response.nProcessed);

					return response;
				} else {
					ESP_LOGV(Bdg::kDebugTag, "Routing::operator()(UART) response ignored");
				}
			}
#endif

#if CONFIG_WIFI_UART_BRIDGE_UART_MAVLINK_FORWARD
			// Message has not been claimed, and is required to be forwarded
			if (Sock::Api::checkInstance()) {
				asio::error_code err;
				auto &socketApi = Sock::Api::getInstance();

				for (const auto &endpoint : container.udpEndpoints) {
					std::uint16_t port = static_cast<std::uint16_t>(Udp::Mavlink);
					socketApi.sendTo(endpoint, port, aUart.payload, err);
				}
			}
#endif

			return {Sub::Rout::Response::Type::Ignored};

		default:

			return {Sub::Rout::Response::Type::Ignored};
	}
}

Sub::Rout::Response Routing::operator()(const Sub::Rout::Socket<asio::ip::tcp> &aTcp)
{
	switch (static_cast<Tcp>(aTcp.localPort)) {
		default: {
			for (auto &callable : Sub::Rout::MavlinkPackForward<asio::ip::tcp>::getIterators()) {

				// Forward MAVLink-wrapped payload iteratively
				auto tcp = aTcp;
				for (auto nRemaining = tcp.payload.size(); nRemaining; nRemaining = tcp.payload.size()) {

					auto nresponse = callable(tcp);

#if CONFIG_WIFI_UART_BRIDGE_TCP_UNNAMED_FORWARD_UART_MAVLINK
					Sub::Rout::UartSend::notify({nresponse.payload, static_cast<int>(Uart::Mavlink)});
#endif

#if CONFIG_WIFI_UART_BRIDGE_TCP_UNNAMED_FORWARD_UDP_MAVLINK
					for (const auto &udpEnpoint : container.udpEndpoints) {
						auto port = static_cast<std::uint16_t>(Udp::Mavlink);
						asio::error_code err;
						Sock::Api::getInstance().sendTo(udpEnpoint, port, nresponse.payload, err);
					}
#endif

					ESP_LOGV(Bdg::kDebugTag, "Routing(TCP), nprocessed: %d", nresponse.nProcessed);

					// Slice payload
					tcp.payload = Utility::makeAsioCb(
						Utility::toBuffer<const void>(tcp.payload).asSlice(nresponse.nProcessed));
				}
			}

			return {Sub::Rout::Response::Type::Consumed};
		}
	}
}

Sub::Rout::Response Routing::operator()(const Sub::Rout::Socket<asio::ip::udp> &aUdp)
{
	switch (static_cast<Udp>(aUdp.localPort)) {

		case Udp::Mavlink: {
			auto it = std::find(container.udpEndpoints.begin(), container.udpEndpoints.end(), aUdp.remoteEndpoint);
			if (container.udpEndpoints.end() == it) {
				container.udpEndpoints.emplace_back(aUdp.remoteEndpoint);  // Remember the client
			}

#if CONFIG_WIFI_UART_BRIDGE_UDP_MAVLINK_PROCESS
			for (auto &callable : Sub::Rout::OnMavlinkReceived::getIterators()) {
				auto response = callable(aUdp.payload);

				if (Sub::Rout::Response::Type::Ignored != response.getType()) {
					return response;
				}
			}
#endif

#if CONFIG_WIFI_UART_BRIDGE_UDP_MAVLINK_FORWARD
			Sub::Rout::UartSend::notify(Sub::Rout::Uart{aUdp.payload,
				static_cast<decltype(Sub::Rout::Uart::uartNum)>(Uart::Mavlink)});
#endif

			return {Sub::Rout::Response::Type::Ignored};
		}
		default:

			return {Sub::Rout::Response::Type::Ignored};
	}
}

Sub::Rout::Response Routing::operator()(const Sub::Rout::Mavlink &aMavlinkMessage)
{
#if CONFIG_WIFI_UART_BRIDGE_UART_MAVLINK_FORWARD  // TODO: change naming
	if (Sock::Api::checkInstance()) {
		for (auto &udpEndpoint : container.udpEndpoints) {
			auto port = static_cast<std::uint16_t>(Udp::Mavlink);
			asio::error_code err;
			Sock::Api::getInstance().sendTo(udpEndpoint, port, aMavlinkMessage.payload, err, asio::ip::udp::v4());

			if (err) {
				ESP_LOGE(Bdg::kDebugTag, "Routing:onReceived(Mavlink) - could not send over UDP, error (%d)",
					err.value());
			}
		}
	}
#endif

	Sub::Rout::UartSend::notify(Sub::Rout::Uart{aMavlinkMessage.payload,
		static_cast<decltype(Sub::Rout::Uart::uartNum)>(Uart::Mavlink)});

	return {Sub::Rout::Response::Type::Consumed};
}

Sub::Rout::OnReceived::Ret Routing::onReceived(Sub::Rout::OnReceived::Arg<0> aVariant)
{
	GS_UTILITY_LOG_SECTIONV(Bdg::kDebugTag, "Routing:onReceived");
	return mapbox::util::apply_visitor(*this, aVariant.variant);
}

Sub::Rout::OnTcpEvent::Ret Routing::onTcpEvent(Sub::Rout::OnTcpEvent::Arg<0> aTcpEventVariant)
{
	GS_UTILITY_LOG_SECTIONV(Bdg::kDebugTag, "Routing:onTcpEvent");

	for (auto &callable : Sub::Rout::MavlinkPackTcpEvent::getIterators()) {
		auto response = callable(aTcpEventVariant);

		if (Sub::Rout::Response::Type::Ignored != response.getType()) {
			Sub::Rout::UartSend::notify(Sub::Rout::Uart{response.payload,
				static_cast<decltype(Sub::Rout::Uart::uartNum)>(Uart::Mavlink)});
		}

#if CONFIG_WIFI_UART_BRIDGE_TCP_CONNECTION_EVENT_FORWARD_UDP_MAVLINK
		if (Sock::Api::checkInstance()) {
			asio::error_code err;
			auto &socketApi = Sock::Api::getInstance();

			for (const auto &endpoint : container.udpEndpoints) {
				std::uint16_t port = static_cast<std::uint16_t>(Udp::Mavlink);
				socketApi.sendTo(endpoint, port, response.payload, err);
			}
		}
#endif
	}
}

Routing::Routing() : key{{&Routing::onReceived, this}, {&Routing::onTcpEvent, this}}
{
}
