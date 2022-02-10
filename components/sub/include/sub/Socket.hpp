//
// Socket.hpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_INCLUDE_SUB_SOCKET_HPP
#define SUB_INCLUDE_SUB_SOCKET_HPP

#include <Rr/Module.hpp>
#include <asio.hpp>
#include <cstdint>
#include "sub/Subscription.hpp"

namespace Sub {
namespace Socket {

enum class Cmd {
	Received,
};

template <class Tproto, Cmd>
struct Tcommand {
	asio::ip::basic_endpoint<Tproto> endpoint;
	std::uint16_t port;  ///< Local port
};

template <class Tproto, Cmd Icmd>
struct TretCommand {  ///< Return type
	asio::error_code errorCode;
	std::uint16_t port;  ///< Opened port
};

template <class Tproto, Cmd Icmd>
using Key = typename Sub::IndModule< TretCommand<Tproto, Icmd>(const Tcommand<Tproto, Icmd> &)>;

}  // namespace Socket
}  // namespace Sub

#endif  // SUB_INCLUDE_SUB_SOCKET_HPP
