//
// socket.hpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SOCKET_INCLUDE_SOCKET_SOCKET_HPP
#define SOCKET_INCLUDE_SOCKET_SOCKET_HPP

#include <asio.hpp>

namespace Sock {
void init();
void init(asio::io_context &);
}  // namespace Socket

#endif // SOCKET_INCLUDE_SOCKET_SOCKET_HPP
