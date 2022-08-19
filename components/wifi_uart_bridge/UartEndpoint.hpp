//
// UartEndpoint.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_WIFI_UART_BRIDGE_UARTENDPOINT_HPP
#define COMPONENTS_WIFI_UART_BRIDGE_UARTENDPOINT_HPP

#include "Endpoint.hpp"
#include "utility/system/UartDevice.hpp"

class UartEndpoint : private Utility::Sys::UartDevice, public Endpoint {
public:
	using UartDevice::UartDevice;

	size_t read(asio::mutable_buffer) override;
	size_t write(asio::const_buffer) override;
};

#endif  // COMPONENTS_WIFI_UART_BRIDGE_UARTENDPOINT_HPP
