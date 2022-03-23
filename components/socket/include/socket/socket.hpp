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

void start();
void start(asio::io_context &);

static constexpr const char *kDebugTag = "[socket]";

}  // namespace Socket

#endif // SOCKET_INCLUDE_SOCKET_SOCKET_HPP
