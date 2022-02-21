//
// Routing.cpp
//
// Created on: Feb 11, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include "Routing.hpp"
#include "socket/Api.hpp"

using namespace Bdg;

Sub::Rout::Response Routing::operator()(const Sub::Rout::Uart &aUart)
{
	return {Sub::Rout::Response::Type::Ignored};

	switch (static_cast<Uart>(aUart.uartNum)) {
		case Uart::Mavlink:
			for (auto &callable : Sub::Rout::OnMavlinkReceived::getIterators()) {
				auto response = callable(aUart.payload);  // Only 1 module serving MAVLink events is expected to be attached

				if (response.getType() == Sub::Rout::Response::Type::Ignored) {  // Message has not been claimed. Forward

					if (!Sock::Api::checkInstance()) {
						return {Sub::Rout::Response::Type::Ignored};
					}

					asio::error_code err;
					auto &socketApi = Sock::Api::getInstance();

					for (const auto &endpoint : container.udpEndpoints) {
						std::uint16_t port = static_cast<std::uint16_t>(Udp::Mavlink);
						socketApi.sendTo(endpoint, port, aUart.payload, err);
					}
				} else {
					return response;
				}

			}

			return {Sub::Rout::Response::Type::Ignored};

		default:

			return {Sub::Rout::Response::Type::Ignored};
	}
}

Sub::Rout::Response Routing::operator()(const Sub::Rout::Socket<asio::ip::tcp> &)
{
	return {Sub::Rout::Response::Type::Ignored};
}

Sub::Rout::Response Routing::operator()(const Sub::Rout::Socket<asio::ip::udp> &aUdp)
{
	switch (static_cast<Udp>(aUdp.localPort)) {

		case Udp::Mavlink: {
			auto it = std::find(container.udpEndpoints.begin(), container.udpEndpoints.end(), aUdp.remoteEndpoint);
			if (container.udpEndpoints.end() == it) {
				container.udpEndpoints.emplace_back(aUdp.remoteEndpoint);  // Remember the client
			}

			for (auto &callable : Sub::Rout::OnMavlinkReceived::getIterators()) {
				auto response = callable(aUdp.payload);

				if (response.getType() == Sub::Rout::Response::Type::Ignored) {  // Message has not been claimed. Forward.
					Sub::Rout::UartSend::notify(Sub::Rout::Uart{aUdp.payload,
						static_cast<decltype(Sub::Rout::Uart::uartNum)>(Uart::Mavlink)});
				} else {
					return response;
				}
			}

			return {Sub::Rout::Response::Type::Ignored};
		}
		default:

			return {Sub::Rout::Response::Type::Ignored};
	}
}

Sub::Rout::OnReceived::Ret Routing::onReceived(Sub::Rout::OnReceived::Arg<0> aVariant)
{
	return mapbox::util::apply_visitor(*this, aVariant);
}

Routing::Routing() : key{{&Routing::onReceived, this}}
{
}
