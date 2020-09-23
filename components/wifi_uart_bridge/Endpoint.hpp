#ifndef COMPONENTS_WIFI_UART_BRIDGE_ENDPOINT_HPP
#define COMPONENTS_WIFI_UART_BRIDGE_ENDPOINT_HPP

#include <asio.hpp>

class Endpoint {
public:
	virtual size_t read(asio::mutable_buffer) = 0;
	virtual size_t write(asio::const_buffer) = 0;

	virtual ~Endpoint()
	{
	}
};

#endif  // COMPONENTS_WIFI_UART_BRIDGE_ENDPOINT_HPP
