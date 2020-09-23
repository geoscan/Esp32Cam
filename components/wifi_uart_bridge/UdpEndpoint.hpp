//
// UdpEndpoint.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_WIFI_UART_BRIDGE_UDPENDPOINT_HPP
#define COMPONENTS_WIFI_UART_BRIDGE_UDPENDPOINT_HPP

#include <esp_timer.h>

#include <asio.hpp>
#include <map>
#include <utility>

#include "Endpoint.hpp"

class UdpClients;

class UdpEndpoint : public Endpoint {
public:
	size_t read(asio::mutable_buffer) override;
	void   write(asio::const_buffer) override;

	UdpEndpoint(asio::io_context, unsigned port, const unsigned nMaxClients = 1, const unsigned timeoutNoInputSec);
private:
	using CliEndpoint = asio::ip::udp::endpoint;
	using Time        = decltype(esp_timer_get_time());

	bool expired(Time) const;
	bool tryAccept(CliEndpoint);

	const Time                  kTimeout;
	asio::ip::udp::socket       socket;
	std::map<CliEndpoint, Time> clients;
};

#endif  // COMPONENTS_WIFI_UART_BRIDGE_UDPENDPOINT_HPP
