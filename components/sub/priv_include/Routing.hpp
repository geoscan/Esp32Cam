//
// Routing.hpp
//
// Created on: Feb 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_PRIV_INCLUDE_ROUTING_HPP
#define SUB_PRIV_INCLUDE_ROUTING_HPP

#include "sub/Rout.hpp"
#include <list>

namespace Sub {

class Routing {

private:

	enum class Uart {
		Mavlink = 1,  // Mavlink, etc.
	};

	enum class Udp {  ///< Named UDP ports
		Mavlink = 8001,
	};

public:
	Rout::Response operator()(const Rout::Uart &);
	Rout::Response operator()(const Rout::Socket<asio::ip::tcp> &);
	Rout::Response operator()(const Rout::Socket<asio::ip::udp> &);
	Rout::Response onReceived(const Rout::ReceivedVariant &);

private:
	struct {
		std::list<asio::ip::udp::endpoint> udpEndpoints;  ///> Memoized clients
	} container;
};

}  // namespace Sub

#endif // SUB_PRIV_INCLUDE_ROUTING_HPP
