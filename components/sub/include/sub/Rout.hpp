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

#include "utility/Buffer.hpp"
#include "sub/Subscription.hpp"
#include <asio.hpp>
#include <mapbox/variant.hpp>
#include <memory>

namespace Sub {
namespace Rout {

using PayloadHold = std::unique_ptr<std::uint8_t[]>;
using Payload = asio::const_buffer;

template <class Tproto>
struct Socket {
	asio::ip::basic_endpoint<Tproto> remoteEndpoint;
	std::uint16_t localPort;
	Payload payload;
};

struct Uart {
	Payload payload;
	int uartNum;
};

struct Response {
	Payload payload;
	PayloadHold payloadHold;

	enum class Type {
		Ignored = 1,
		Acknowledged,
		Response,
	};

	Type getType();
	void setType(Type);
};

namespace Topic {
struct Generic;
struct Mavlink;
}  // namespace Topic

using ReceivedVariant = typename mapbox::util::variant<Socket<asio::ip::udp>, Socket<asio::ip::tcp>, Uart>;
using OnMavlinkReceived = Sub::NoLockKey<Response(Payload), Topic::Mavlink>;
using OnReceived = Sub::IndKey<Response(const ReceivedVariant &), Topic::Generic>;  ///< Event signifying that something has been received
using UartSend = Sub::IndKey<void(const Uart &), Topic::Generic>;  ///< Command to send a packet over UART. TODO: consider the same approach for Sock::Api, when RR library gets mature enough so it enables one to register interfaces inst. of function callbacks

}  // namespace Rout
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_ROUTING_HPP
