//
// Endpoint.hpp
//
// Created on: Mar 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef DEBUG_UDP_PRIVATE_INCLUDE_ENDPOINT_HPP
#define DEBUG_UDP_PRIVATE_INCLUDE_ENDPOINT_HPP

#include <asio.hpp>
#include <forward_list>

namespace LogUdp {

class Endpoint {
public:
	Endpoint(asio::ip::udp::socket &);
	void operator()();
	void send(const char *);
private:
	asio::ip::udp::socket &socket;
	std::forward_list<asio::ip::udp::endpoint> endpoints;
};

}  // namespace DebugUdp

#endif // DEBUG_UDP_PRIVATE_INCLUDE_ENDPOINT_HPP
