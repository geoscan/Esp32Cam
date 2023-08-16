//
// Routing.hpp
//
// Created on: Feb 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// Topics for forwarding messages between serial, network, or other interfaces.
//

#ifndef SUB_INCLUDE_SUB_ROUTING_HPP
#define SUB_INCLUDE_SUB_ROUTING_HPP

#include "utility/cont/Buffer.hpp"
#include "sub/Subscription.hpp"
#include <asio.hpp>
#include <mapbox/variant.hpp>
#include <memory>
#include <mutex>

namespace Sub {
namespace Rout {

using PayloadHold = std::unique_ptr<std::uint8_t[]>;
using PayloadLock = std::unique_ptr<std::lock_guard<std::mutex>>;
using Payload = asio::const_buffer;

///
/// \brief Used by Sock::Api
///
struct TcpConnected {
	const asio::ip::tcp::endpoint remoteEndpoint;
	std::uint16_t localPort;
};

struct Uart {
	Payload payload;
	int uartNum;
};

///
/// \brief Used by Sock::Api
///
struct TcpDisconnected {
	const asio::ip::tcp::endpoint remoteEndpoint;
	std::uint16_t localPort;
};

namespace Topic {
struct Generic;
struct Mavlink;
}  // namespace Topic

using TcpConnectionVariant = typename mapbox::util::variant<TcpConnected, TcpDisconnected>;  ///< Tcp connection events

using OnTcpEvent = Sub::NoLockKey<void(const TcpConnectionVariant &), Topic::Generic>;  ///< Used by Routing. TODO: if number of users is extended, replace w/ Sub::IndKey!
using UartSend = Sub::NoLockKey<void(const Uart &), Topic::Generic>;  ///< Command to send a packet over UART. TODO: consider the same approach for Sock::Api, when RR library gets mature enough so it enables one to register interfaces inst. of function callbacks

}  // namespace Rout
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_ROUTING_HPP
