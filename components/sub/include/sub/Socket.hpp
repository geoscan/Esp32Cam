//
// Socket.hpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_INCLUDE_SUB_SOCKET_HPP
#define SUB_INCLUDE_SUB_SOCKET_HPP

#include <asio.hpp>
#include <cstdint>
#include "sub/Subscription.hpp"
#include <memory>

namespace Sub {
namespace Socket {

// Process received

template <class Tproto>
struct ArgReceived {
	asio::ip::basic_endpoint<Tproto> sender;
	std::uint16_t localPort;
	asio::const_buffer payload;
};

struct RetReceived {
	asio::const_buffer response;
	std::unique_ptr<char[]> bufferHold;
};

template <class Tproto>
using Received = typename Sub::IndKey<RetReceived(const ArgReceived<Tproto> &)>;

}  // namespace Socket
}  // namespace Sub

#endif  // SUB_INCLUDE_SUB_SOCKET_HPP
