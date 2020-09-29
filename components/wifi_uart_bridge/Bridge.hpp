//
// Bridge.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP
#define COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP

#include <functional>
#include <utility>
#include "Endpoint.hpp"

class Bridge {
public:
	Bridge(Endpoint&, Endpoint&);
	void run();
private:
	static constexpr const unsigned kBufSize = 128;

	Endpoint &first;
	Endpoint &second;

	using EpPair = std::pair<Endpoint *, Endpoint *>;

	static void *bridgeTask(void *endpoints);
	static void performTransfer(Endpoint &, Endpoint &, asio::mutable_buffer);
};

#endif // COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP
