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
	size_t write(asio::const_buffer) override;

	UdpEndpoint(asio::io_context &, uint16_t port = CONFIG_WIFI_UART_BRIDGE_UDP_PORT,
		size_t nMaxClients = CONFIG_WIFI_UART_BRIDGE_MAX_NCLIENTS,
		size_t timeoutNoInputSec = CONFIG_WIFI_UART_BRIDGE_INACTIVE_TIMEOUT_SECONDS);
private:

	using CliEndpoint = asio::ip::udp::endpoint;
	using Time        = decltype(esp_timer_get_time());

	bool expired(Time) const;
	bool tryAccept(CliEndpoint);
	void lock();
	void unlock();

	const Time                  kTimeout;  // us
	const size_t                kMaxClients;
	asio::ip::udp::socket       socket;
	std::map<CliEndpoint, Time> clients;
	asio::detail::mutex         mutex;
};

#endif  // COMPONENTS_WIFI_UART_BRIDGE_UDPENDPOINT_HPP
