//
// Api.cpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "socket/Api.hpp"
#include "sub/Rout.hpp"

namespace Sock {

Api::Api(asio::io_context &aIoContext, std::mutex &aSyncAsyncMutex):
	MakeSingleton<Api>{*this},
	ioContext{aIoContext},
	syncAsyncMutex{aSyncAsyncMutex}
{
}

void Api::connect(const asio::ip::tcp::endpoint &aRemoteEndpoint, uint16_t &aLocalPort, asio::error_code &aErr,
	asio::ip::tcp aTcp)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpConnected.find(aRemoteEndpoint, aLocalPort);

	if (it != container.tcpConnected.end()) {
		aErr = asio::error::already_connected;
	} else {
		if (aLocalPort != 0) {
			container.tcpConnected.emplace_back(ioContext, asio::ip::tcp::endpoint{aTcp, aLocalPort});
		} else {
			container.tcpConnected.emplace_back(ioContext);
			aLocalPort = container.tcpConnected.back().local_endpoint().port();
		}

		container.tcpConnected.back().connect(aRemoteEndpoint, aErr);

		if (aErr) {
			container.tcpConnected.back().close();
			container.tcpConnected.pop_back();
		} else {
			tcpAsyncReceiveFrom(container.tcpConnected.back());
		}
	}

}

void Api::disconnect(const asio::ip::tcp::endpoint &aRemoteEndpoint, std::uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpConnected.find(aRemoteEndpoint, aPort);

	if (it != container.tcpConnected.end()) {
		aErr = asio::error::not_connected;
	} else {
		it->shutdown(asio::ip::tcp::socket::shutdown_both, aErr);
		it->close(aErr);
	}
}

void Api::openTcp(uint16_t aLocalPort, asio::error_code &aErr, asio::ip::tcp aTcp)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpListening.find(aLocalPort);

	if (it != container.tcpListening.end()) {
		aErr = asio::error::already_open;
	} else {
		container.tcpListening.emplace_back(ioContext, asio::ip::tcp::endpoint{aTcp, aLocalPort});
	}
}

void Api::openUdp(uint16_t aLocalPort, asio::error_code &aErr, asio::ip::udp aUdp)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.udp.find(aLocalPort);

	if (it != container.udp.end()) {
		aErr = asio::error::already_open;
	} else {
		container.udp.emplace_back(ioContext, asio::ip::udp::endpoint{aUdp, aLocalPort});
		udpAsyncReceiveFrom(container.udp.back());
	}
}

void Api::closeUdp(uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.udp.find(aPort);

	if (it == container.udp.end()) {
		aErr = asio::error::not_found;
	} else {
		it->close(aErr);
	}
}

void Api::closeTcp(uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpListening.find(aPort);

	if (it == container.tcpListening.end()) {
		aErr = asio::error::not_found;
	} else {
		it->close(aErr);
	}
}

void Api::udpAsyncReceiveFrom(asio::ip::udp::socket &aSocket)
{
	std::shared_ptr<char[]> buffer {new char[kReceiveBufferSize]};
	auto endpoint = std::make_shared<asio::ip::udp::endpoint>();
	auto port = aSocket.local_endpoint().port();

	aSocket.async_receive_from(asio::buffer(buffer.get(), kReceiveBufferSize), *endpoint.get(),
		[this, buffer, endpoint, port, &aSocket] (asio::error_code aError, std::size_t anReceived) mutable {

		if (!aError) {
			for (auto &cb : Sub::Rout::OnReceived::getIterators()) { // Notify subscribers
				auto response = cb(Sub::Rout::Socket<asio::ip::udp>{
					*endpoint.get(),
					port,
					asio::const_buffer(buffer.get(), anReceived)
				});

				// If a subscriber provides a response, send it
				if (response.getType() == Sub::Rout::Response::Type::Response) {
					std::lock_guard<std::mutex> lock{syncAsyncMutex};
					(void)lock;
					asio::error_code err;
					aSocket.send_to(response.payload, *endpoint.get(), 0, err);
				}
			}
		}

		udpAsyncReceiveFrom(aSocket);
	});
}

void Api::tcpAsyncReceiveFrom(asio::ip::tcp::socket &aSocket)
{
	std::shared_ptr<char[]> buffer{new char[kReceiveBufferSize]};

	aSocket.async_receive(asio::buffer(buffer.get(), kReceiveBufferSize),
		[this, buffer, &aSocket](const asio::error_code &aErr, std::size_t anReceived) mutable {

		if (!aErr) {
			for (auto &cb : Sub::Rout::OnReceived::getIterators()) {  // Notify subscribers
				auto response = cb(Sub::Rout::Socket<asio::ip::tcp>{
					aSocket.remote_endpoint(),
					aSocket.local_endpoint().port(),
					asio::const_buffer(buffer.get(), anReceived)
				});

				// If a subscriber provides a response, send it
				if (response.getType() == Sub::Rout::Response::Type::Response) {
					asio::error_code err;
					aSocket.write_some(response.payload, err);
				}
			}
		}

		tcpAsyncReceiveFrom(aSocket);
	});
}

}  // namespace Sock
