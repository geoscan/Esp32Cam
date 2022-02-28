﻿//
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
#include <mutex>

namespace Sub {
namespace Rout {

using PayloadHold = std::unique_ptr<std::uint8_t[]>;
using PayloadLock = std::unique_ptr<std::lock_guard<std::mutex>>;
using Payload = asio::const_buffer;

template <class Tproto>
struct Socket {
	const asio::ip::basic_endpoint<Tproto> &remoteEndpoint;
	std::uint16_t localPort;
	Payload payload;
};

struct Uart {
	Payload payload;
	int uartNum;
};

struct Response {
	Payload payload;
	PayloadHold payloadHold;  ///< Storage for payload. Solves the transferred buffer's lifetime problem in cases when a handler does not possess the buffer.
	int nProcessed;  ///< Number of bytes that has been processed by a handler. For iterative use (e.g. sending long MAVLink message chunk-by-chunk). Contextual, may be unused (= -1 in that case)
	PayloadLock payloadLock;  ///< Payload may refer to a buffer owned by a handler. This field allows to protect the buffer until `payload` is processed by an invoker.

	enum class Type {
		Ignored,  ///< The message has not been recognized as the one that was addressed to a receiver
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

using ReceivedVariant = typename mapbox::util::variant<Socket<asio::ip::udp>, Socket<asio::ip::tcp>, Uart>;
using OnMavlinkReceived = Sub::NoLockKey<Response(Payload), Topic::Mavlink>;
using OnReceived = Sub::IndKey<Response(const ReceivedVariant &), Topic::Generic>;  ///< Event signifying that something has been received
using UartSend = Sub::NoLockKey<void(const Uart &), Topic::Generic>;  ///< Command to send a packet over UART. TODO: consider the same approach for Sock::Api, when RR library gets mature enough so it enables one to register interfaces inst. of function callbacks

template <class Tproto>
using MavlinkPackForward = typename Sub::NoLockKey<Response(const Socket<Tproto> &), Topic::MavlinkPackForward>;

}  // namespace Rout
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_ROUTING_HPP
