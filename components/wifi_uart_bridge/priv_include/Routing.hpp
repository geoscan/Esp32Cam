//
// Routing.hpp
//
// Created on: Feb 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_PRIV_INCLUDE_ROUTING_HPP
#define SUB_PRIV_INCLUDE_ROUTING_HPP

#include "sub/Rout.hpp"
#include "utility/CircularBuffer.hpp"

namespace Bdg {

class Routing {
private:

	enum class Uart {
		Mavlink = 0,  // Mavlink, etc.
	};

	enum class Udp {  ///< Named UDP ports
		Mavlink = 8001,
	};

	enum class Tcp {  ///< Named TCP ports
	};

public:
	Sub::Rout::Response operator()(const Sub::Rout::Uart &);
	Sub::Rout::Response operator()(const Sub::Rout::Socket<asio::ip::tcp> &);
	Sub::Rout::Response operator()(const Sub::Rout::Socket<asio::ip::udp> &);
	Sub::Rout::Response operator()(const Sub::Rout::Mavlink &);
	Sub::Rout::OnReceived::Ret onReceived(Sub::Rout::OnReceived::Arg<0>);
	Sub::Rout::OnTcpEvent::Ret onTcpEvent(Sub::Rout::OnTcpEvent::Arg<0>);
	Routing();

private:
	static constexpr auto knUdpClients = 2;
	static constexpr auto kfOverwriteUdpClients = true;
	struct {
		Utility::CircularBuffer<asio::ip::udp::endpoint, knUdpClients, kfOverwriteUdpClients> udpEndpoints;  ///> Memoized clients
	} container;

	struct {
		Sub::Rout::OnReceived onReceived;
		Sub::Rout::OnTcpEvent onTcpEvent;
	} key;
};

}  // namespace Bdg

#endif // SUB_PRIV_INCLUDE_ROUTING_HPP
