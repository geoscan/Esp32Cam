//
// Routing.hpp
//
// Created on: Feb 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_PRIV_INCLUDE_ROUTING_HPP
#define SUB_PRIV_INCLUDE_ROUTING_HPP

#include "sub/Rout.hpp"
#include <set>

namespace Bdg {

class Routing {
private:

	enum class Uart {
		Mavlink = 0,  // Mavlink, etc.
	};

	enum class Udp {  ///< Named UDP ports
		Mavlink = 8001,
	};

public:
	Sub::Rout::Response operator()(const Sub::Rout::Uart &);
	Sub::Rout::Response operator()(const Sub::Rout::Socket<asio::ip::tcp> &);
	Sub::Rout::Response operator()(const Sub::Rout::Socket<asio::ip::udp> &);
	Sub::Rout::Response onReceived(const Sub::Rout::ReceivedVariant &);
	Routing();

private:
	struct {
		std::set<asio::ip::udp::endpoint> udpEndpoints;  ///> Memoized clients
	} container;

	struct {
		Sub::Rout::OnReceived onReceived;
	} key;
};

}  // namespace Bdg

#endif // SUB_PRIV_INCLUDE_ROUTING_HPP
