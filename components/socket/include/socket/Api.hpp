//
// Api.hpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#ifndef SOCKET_INCLUDE_SOCKET_SOCKET_HPP
#define SOCKET_INCLUDE_SOCKET_SOCKET_HPP

#include "utility/MakeSingleton.hpp"
#include "sub/Socket.hpp"
#include "Container.hpp"
#include <mutex>
#include <cstdint>
#include <asio.hpp>

namespace Sock {

class Api : public Utility::MakeSingleton<Api> {
private:
	asio::io_context &ioContext;
	std::mutex &syncAsyncMutex;  ///< Shared b/w reactive requests to this Api, and whatever task that triggers asio::io_context in a loop

	using KeyUdpReceived = typename Sub::Socket::Key<asio::ip::udp, Sub::Socket::Cmd::Received>;
	using KeyTcpReceived = typename Sub::Socket::Key<asio::ip::tcp, Sub::Socket::Cmd::Received>;

	struct {
		Container<asio::ip::tcp::socket> tcpConnected;
		Container<asio::ip::tcp::acceptor> tcpListening;
		Container<asio::ip::udp::socket> udp;
	} container;

public:
	Api(asio::io_context &, std::mutex &aSyncAsyncMutex);
	void connect(const asio::ip::tcp::endpoint &, std::uint16_t &aLocalPort, asio::error_code &aErr);
	void disconnect(const asio::ip::tcp::endpoint &, std::uint16_t aLocalPort, asio::error_code &aErr);
	void openTcp(std::uint16_t aLocalPort, asio::error_code &aErr);  ///< Open a listening socket
	void openUdp(std::uint16_t aLocalPort, asio::error_code &aErr);
	void closeTcp(std::uint16_t aPort, asio::error_code &aErr);
	void closeUdp(std::uint16_t aPort, asio::error_code &aErr);

	template <class Tbuf>
	std::size_t sendTo(const asio::ip::tcp::endpoint &aRemoteEndpoint, std::uint16_t aLocalPort, Tbuf &&, asio::error_code &aErr);

	template <class Tbuf>
	std::size_t sendTo(const asio::ip::udp::endpoint &aRemoteEndpoint, std::uint16_t aLocalPort, Tbuf &&, asio::error_code &aErr);

};

}  // namespace Sock

#include "Api.impl"

#endif // Api_INCLUDE_Api_Api_HPP
