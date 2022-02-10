//
// Api.cpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "socket/Api.hpp"

namespace Sock {

Api::Api(asio::io_context &aIoContext, std::mutex &aSyncAsyncMutex):
	MakeSingleton<Api>{*this},
	ioContext{aIoContext},
	syncAsyncMutex{aSyncAsyncMutex}
{
}

void Api::connect(const asio::ip::tcp::endpoint &aRemoteEndpoint, uint16_t &aLocalPort, asio::error_code &aErr)
{
	static constexpr auto kReceiveBufferSize = 256;
	auto it = container.tcpConnected.find(aRemoteEndpoint, aLocalPort);

	if (it != container.tcpConnected.end()) {
		aErr = asio::error::already_connected;
		return;
	}

	if (aLocalPort != 0) {
		container.tcpConnected.emplace_back(ioContext, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), aLocalPort});
	} else {
		container.tcpConnected.emplace_back(ioContext);
		aLocalPort = container.tcpConnected.back().local_endpoint().port();
	}
	container.tcpConnected.back().connect(aRemoteEndpoint, aErr);

	if (aErr) {
		container.tcpConnected.back().close();
		container.tcpConnected.pop_back();
		return;
	}

	std::shared_ptr<char[]> buffer{new char[kReceiveBufferSize]};
	container.tcpConnected.back().async_receive(asio::buffer(buffer.get(), kReceiveBufferSize),
		[this, buffer](const asio::error_code &aErrorCode, std::size_t anTransferred) mutable {
		// TODO: event, on received
	});
}

void Api::disconnect(const asio::ip::tcp::endpoint &aRemoteEndpoint, std::uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	auto it = container.tcpConnected.find(aRemoteEndpoint, aPort);
	(void)lock;

	if (it != container.tcpConnected.end()) {
		aErr = asio::error::not_connected;
		return;
	}

	it->shutdown(asio::ip::tcp::socket::shutdown_both, aErr);
	it->close(aErr);
}

void Api::openTcp(uint16_t aLocalPort, asio::error_code &aErr)
{
	auto it = container.tcpListening.find(aLocalPort);

	if (it != container.tcpListening.end()) {
		aErr = asio::error::already_started;
	}

	container.tcpListening.emplace_back(ioContext, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), aLocalPort});
}

void Api::closeUdp(uint16_t aPort, asio::error_code &aErr)
{
	auto it = container.udp.find(aPort);

	if (it == container.udp.end()) {
		aErr = asio::error::not_found;
	}

	{
		std::lock_guard<std::mutex> lock{syncAsyncMutex};
		it->close(aErr);
	}
}

void Api::closeTcp(uint16_t aPort, asio::error_code &aErr)
{
	auto it = container.tcpListening.find(aPort);

	if (it == container.tcpListening.end()) {
		aErr = asio::error::not_found;
		return;
	}

	{
		std::lock_guard<std::mutex> lock{syncAsyncMutex};
		it->close(aErr);
	}
}

}  // namespace Sock
