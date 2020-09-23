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
	Endpoint &first;
	Endpoint &second;

	using Endpoints = std::pair<std::reference_wrapper<Endpoint> /*from*/, std::reference_wrapper<Endpoint> /*to*/>;

	static void *bridgingRoutine(void *endpoints);
};

#endif // COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP
