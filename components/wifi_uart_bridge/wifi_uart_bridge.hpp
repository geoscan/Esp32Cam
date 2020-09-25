//
// wifi_uart_bridge_udp.h
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_H
#define COMPONENTS_WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_H

#include <asio.hpp>

void wifiUartBridgeStart(asio::io_context &);

#endif // COMPONENTS_WIFI_UART_BRIDGE_WIFI_UART_BRIDGE_H
