//
// Endpoint.hpp
//
// Created on: Mar 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef DEBUG_UDP_PRIVATE_INCLUDE_ENDPOINT_HPP
#define DEBUG_UDP_PRIVATE_INCLUDE_ENDPOINT_HPP

#include <asio.hpp>
#include <list>
#include "sub/Subscription.hpp"

namespace LogUdp {

class Endpoint {
public:
	Endpoint(asio::ip::udp::socket &);
	void operator()();
	void send(const char *);
private:
	void onWifiDisconnected(Sub::Key::WifiDisconnected::Type);

	std::mutex mut;
	Sub::Key::WifiDisconnected keyWifiDisconnected;
	asio::ip::udp::socket &socket;
	std::list<asio::ip::udp::endpoint> endpoints;
};

}  // namespace DebugUdp

#endif // DEBUG_UDP_PRIVATE_INCLUDE_ENDPOINT_HPP
