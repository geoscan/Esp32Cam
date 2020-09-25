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
#include <functional>

#include "Endpoint.hpp"

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
	using CliInfo     = std::pair<CliEndpoint, Time>;


	// ------------ CliStack ------------ //


	class CliStack {
	public:
		CliStack(std::list<CliInfo>::iterator incrementIterator, size_t clients);
		CliInfo &pop();
		bool empty() const;
	private:
		std::list<CliInfo>::iterator iter;
		size_t size;
	};


	// ------------ CliMap ------------ //


	class CliMap {
	public:
		Time &at(CliEndpoint);
		bool contains(CliEndpoint);
		void set(CliEndpoint, Time);  // Checks presence of the requested endpoint, adds it if it's not there
		CliStack stack();
	private:
		std::map<CliEndpoint, std::reference_wrapper<CliInfo>> cliMap;
		std::list<CliInfo> cliStack;
		asio::detail::mutex mutex;  // Primarily used for integrity of cliMap
	};


	static Time time();

	bool expired(Time) const;

	// Semaphore modifiers
	bool addClient(bool fAdd);

	const Time            kTimeout;  // us
	const size_t          kMaxClients;
	SemaphoreHandle_t     semaphore;  // Counter of active clients
	CliMap                cliMap;
	asio::ip::udp::socket socket;
};


#endif  // COMPONENTS_WIFI_UART_BRIDGE_UDPENDPOINT_HPP
