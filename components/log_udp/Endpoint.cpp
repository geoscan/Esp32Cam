//
// Endpoint.cpp
//
// Created on: Mar 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <sdkconfig.h>
#include "private_include/Endpoint.hpp"

LogUdp::Endpoint::Endpoint(asio::ip::udp::socket &aSocket) :
	keyWifiDisconnected{&Endpoint::onWifiDisconnected, this},
	socket(aSocket)
{
}

void LogUdp::Endpoint::operator ()()
{
	asio::ip::udp::endpoint endpoint;
	char stub;
	while (true) {
		socket.receive_from(asio::mutable_buffer{&stub, 1}, endpoint);
		std::lock_guard<std::mutex> lock{mut};
		endpoints.push_front(endpoint);
	}
}

void LogUdp::Endpoint::send(const char *msg)
{
	asio::error_code err;
	std::lock_guard<std::mutex> lock{mut};
	for (auto &endpoint : endpoints) {
		socket.send_to(asio::const_buffer{msg, strlen(msg)}, endpoint, 0, err);
	}
}

void LogUdp::Endpoint::onWifiDisconnected(Sub::Key::WifiDisconnected::Type aAddress)
{
	std::lock_guard<std::mutex> lock{mut};
	endpoints.clear();
}