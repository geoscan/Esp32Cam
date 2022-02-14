//
// Api.hpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#ifndef SOCKET_INCLUDE_SOCKET_API_HPP
#define SOCKET_INCLUDE_SOCKET_API_HPP

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

	struct {
		Container<asio::ip::tcp::socket> tcpConnected;
		Container<asio::ip::tcp::acceptor> tcpListening;
		Container<asio::ip::udp::socket> udp;
	} container;

	static constexpr auto kReceiveBufferSize = 128;

public:
	Api(asio::io_context &, std::mutex &aSyncAsyncMutex);
	void connect(const asio::ip::tcp::endpoint &, std::uint16_t &aLocalPort, asio::error_code &aErr,
		asio::ip::tcp = asio::ip::tcp::v4());
	void disconnect(const asio::ip::tcp::endpoint &, std::uint16_t aLocalPort, asio::error_code &aErr);
	void openTcp(std::uint16_t aLocalPort, asio::error_code &aErr, asio::ip::tcp = asio::ip::tcp::v4());  ///< Open a listening socket
	void openUdp(std::uint16_t aLocalPort, asio::error_code &aErr, asio::ip::udp = asio::ip::udp::v4());
	void closeTcp(std::uint16_t aPort, asio::error_code &aErr);
	void closeUdp(std::uint16_t aPort, asio::error_code &aErr);

	template <class Tbuf>
	std::size_t sendTo(const asio::ip::tcp::endpoint &aRemoteEndpoint, std::uint16_t aLocalPort, Tbuf &&,
		asio::error_code &aErr);

	template <class Tbuf>
	std::size_t sendTo(const asio::ip::udp::endpoint &aRemoteEndpoint, std::uint16_t &aLocalPort, Tbuf &&,
		asio::error_code &aErr, asio::ip::udp = asio::ip::udp::v4());

private:
	void udpAsyncReceiveFrom(asio::ip::udp::socket &aSocket);
	void tcpAsyncReceiveFrom(asio::ip::tcp::socket &aSocket);
};

}  // namespace Sock

#include "Api.impl"

#endif // SOCKET_INCLUDE_SOCKET_API_HPP
