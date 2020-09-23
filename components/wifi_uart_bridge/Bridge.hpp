//
// Bridge.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP
#define COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP

#include "Endpoint.hpp"

class Bridge {
public:
	Bridge(Endpoint&, Endpoint&);
	void run();
};

#endif // COMPONENTS_WIFI_UART_BRIDGE_BRIDGE_HPP
