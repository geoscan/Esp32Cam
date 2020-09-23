//
// UartEndpoint.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_WIFI_UART_BRIDGE_UARTENDPOINT_HPP
#define COMPONENTS_WIFI_UART_BRIDGE_UARTENDPOINT_HPP

#include "Endpoint.hpp"
#include "UartDevice.hpp"

class UartEndpoint : private UartDevice, public Endpoint {
public:
	using UartDevice::UartDevice;

	size_t read(asio::mutable_buffer) override;
	void   write(asio::const_buffer) override;
};

#endif  // COMPONENTS_WIFI_UART_BRIDGE_UARTENDPOINT_HPP
