//
// Api.hpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#ifndef SOCKET_INCLUDE_SOCKET_API_HPP
#define SOCKET_INCLUDE_SOCKET_API_HPP

#include "socket/socket.hpp"
#include "utility/MakeSingleton.hpp"
#include "Container.hpp"
#include <mutex>
#include <cstdint>
#include <asio.hpp>

namespace Sock {

///
/// \brief The Api class Is a wrapper over asio providing convenient interface for working w/ sockets.
///
/// Socket::Api has been originally introduced due to the necessity to decouple MAVLink parsing routines (Geoscan
/// "MAVLink GS_NETWORK" subprotocol) from managing sockets. It provides all the conventional UDP/TCP/IP operations
/// such as connect to a remote endpoint, open a listening socket, etc.
///
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
	static constexpr const char *kDebugTag = Sock::kDebugTag;

public:
	Api(asio::io_context &, std::mutex &aSyncAsyncMutex);
	void connect(const asio::ip::tcp::endpoint &, std::uint16_t &aLocalPort, asio::error_code &aErr,
		asio::ip::tcp = asio::ip::tcp::v4());
	void disconnect(const asio::ip::tcp::endpoint &, std::uint16_t aLocalPort, asio::error_code &aErr);
	void openTcp(std::uint16_t aLocalPort, asio::error_code &aErr, asio::ip::tcp = asio::ip::tcp::v4());  ///< Open a listening socket
	void openUdp(std::uint16_t aLocalPort, asio::error_code &aErr, asio::ip::udp = asio::ip::udp::v4());
	void closeTcp(std::uint16_t aPort, asio::error_code &aErr);
	void closeUdp(std::uint16_t aPort, asio::error_code &aErr);

	std::size_t sendTo(const asio::ip::tcp::endpoint &aRemoteEndpoint, std::uint16_t &aLocalPort, asio::const_buffer,
		asio::error_code &aErr);

	std::size_t sendTo(const asio::ip::udp::endpoint &aRemoteEndpoint, std::uint16_t &aLocalPort, asio::const_buffer,
		asio::error_code &aErr, asio::ip::udp = asio::ip::udp::v4());

private:
	void tcpAsyncAccept(asio::ip::tcp::acceptor &aAcceptor, std::uint16_t aPort);
	void udpAsyncReceiveFrom(asio::ip::udp::socket &aSocket, std::shared_ptr<char[]> buffer = {},
		std::shared_ptr<asio::ip::udp::endpoint> aEndpoint = {});
	void tcpAsyncReceiveFrom(asio::ip::tcp::socket &aSocket, std::shared_ptr<char[]> buffer = {});
};

}  // namespace Sock

#endif // SOCKET_INCLUDE_SOCKET_API_HPP
