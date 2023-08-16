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
/// \brief Identifies TCP or UDP request handlers
///
template <class Tproto>
struct Socket {
	static_assert(std::is_same<Tproto, asio::ip::tcp>::value
		|| std::is_same<Tproto, asio::ip::udp>::value, "");

	const asio::ip::basic_endpoint<Tproto> &remoteEndpoint;
	std::uint16_t localPort;
	Payload payload;
};

///
/// \brief Identifies UART request handlers
///
struct Uart {
	Payload payload;
	int uartNum;
};

/// \brief Mavlink output message. Most likely, it contains a telemetry message
struct Mavlink {
	Payload payload;
};

///
/// \brief Used by Sock::Api
///
struct TcpConnected {
	const asio::ip::tcp::endpoint remoteEndpoint;
	std::uint16_t localPort;
};

///
/// \brief Used by Sock::Api
///
struct TcpDisconnected {
	const asio::ip::tcp::endpoint remoteEndpoint;
	std::uint16_t localPort;
};

///
/// \brief Provides message passing between interfaces and receive-event handlers.
///
/// Please note `payloadLock` and `payloadHold` fields.
/// They enable 2 ways of providing thread safety of `payload` field
/// 1. Allocate a chunk of memory and bind it to self-destructing RAII `std::unique_ptr<bytes[]>` (`payloadHold` field)
/// 2. Use a buffer pre-allocated by the response producer, but lock it w/ `std::unique_ptr<mutex>` (`payloadLock` field)
///
struct Response {
	Payload payload;  ///< Response field
	PayloadHold payloadHold;  ///< Storage for payload. Solves the transferred buffer's lifetime problem in cases when a receiver does not possess the buffer.
	int nProcessed;  ///< Number of bytes that has been processed by a receiver. For iterative use (e.g. sending long MAVLink message chunk-by-chunk). Contextual, may be unused (= -1 in that case)
	PayloadLock payloadLock;  ///< Payload may refer to a buffer owned by a receiver. This field allows to protect the buffer until `payload` is processed by an invoker.

	enum class Type {
		Ignored,  ///< The message has not been recognized by a receiver
		Consumed,  ///< The message was addressed to a receiver. No response is sent
		Response,  ///< The message was addressed to a receiver. There is a response message ready to be sent
	};

	Type getType();
	void setType(Type);
	Response(const Response &) = default;
	Response(Response &&) = default;
	Response &operator=(const Response &) = default;
	Response &operator=(Response &&) = default;
	Response(Type);
	Response();
	void reset();

	template <class Tpayload, class TpayloadHold, class TpayloadLock>
	Response(Tpayload &&aTpayload,
		TpayloadHold &&aTpayloadHold = {},
		int anProcessed = -1,
		TpayloadLock &&aTpayloadLock = {}) :
		payload{std::forward<Tpayload>(aTpayload)},
		payloadHold{std::forward<TpayloadHold>(aTpayloadHold)},
		nProcessed{anProcessed},
		payloadLock{std::forward<TpayloadLock>(aTpayloadLock)}
	{
	}
};

namespace Topic {
struct Generic;
struct Mavlink;
struct MavlinkPackForward;
}  // namespace Topic

using TcpConnectionVariant = typename mapbox::util::variant<TcpConnected, TcpDisconnected>;  ///< Tcp connection events

struct ReceivedVariant {
	using SenderInfoVariant = typename mapbox::util::variant<Socket<asio::ip::udp>, Socket<asio::ip::tcp>, Uart, Mavlink>;  ///< Stores info on whoever produced the request

	SenderInfoVariant variant;
};

using OnTcpEvent = Sub::NoLockKey<void(const TcpConnectionVariant &), Topic::Generic>;  ///< Used by Routing. TODO: if number of users is extended, replace w/ Sub::IndKey!
using UartSend = Sub::NoLockKey<void(const Uart &), Topic::Generic>;  ///< Command to send a packet over UART. TODO: consider the same approach for Sock::Api, when RR library gets mature enough so it enables one to register interfaces inst. of function callbacks

template <class Tproto>
using MavlinkPackForward = typename Sub::NoLockKey<Response(const Socket<Tproto> &), Topic::MavlinkPackForward>;

using MavlinkPackTcpEvent = typename Sub::NoLockKey<Response(const TcpConnectionVariant &), Topic::Mavlink>;

}  // namespace Rout
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_ROUTING_HPP
