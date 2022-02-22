//
// Api.cpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "socket/Api.hpp"
#include "sub/Rout.hpp"
#include "utility/Algorithm.hpp"

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
	Container<asio::ip::tcp::socket>::iterator it;

	if (aLocalPort == 0) {
		it = container.tcpConnected.find(aRemoteEndpoint);
	} else {
		it = container.tcpConnected.find(aRemoteEndpoint, aLocalPort);
	}

	if (it != container.tcpConnected.end()) {
		aErr = asio::error::already_connected;
		ESP_LOGW(kDebugTag, "connect to %s : %d from port %d - already connected",
			aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aLocalPort);
	} else {
		if (aLocalPort != 0) {
			container.tcpConnected.emplace_back(ioContext, asio::ip::tcp::endpoint{aTcp, aLocalPort});
		} else {
			container.tcpConnected.emplace_back(ioContext);
		}

		container.tcpConnected.back().connect(aRemoteEndpoint, aErr);

		if (aErr) {
			ESP_LOGE(kDebugTag, "connect to %s : %d from port %d - error(%d)",
				aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aLocalPort, aErr.value());
			container.tcpConnected.back().close();
			container.tcpConnected.pop_back();
		} else {
			aLocalPort = container.tcpConnected.back().local_endpoint().port();
			ESP_LOGI(kDebugTag, "connect to %s : %d from port %d - success",
				aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aLocalPort);
			tcpAsyncReceiveFrom(container.tcpConnected.back());
		}
	}

}

void Api::disconnect(const asio::ip::tcp::endpoint &aRemoteEndpoint, std::uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpConnected.find(aRemoteEndpoint, aPort);

	if (it == container.tcpConnected.end()) {
		aErr = asio::error::not_connected;
		ESP_LOGW(kDebugTag, "disconnect from %s : %d on port %d - no such socket was found",
			aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aPort);
	} else {
		it->shutdown(asio::ip::tcp::socket::shutdown_both, aErr);
		it->close(aErr);
		container.tcpConnected.erase(it);
		ESP_LOGI(kDebugTag, "disconnect from %s : %d on port %d - success",
			aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aPort);
	}
}

void Api::openTcp(uint16_t aLocalPort, asio::error_code &aErr, asio::ip::tcp aTcp)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpListening.find(aLocalPort);

	if (it != container.tcpListening.end()) {
		ESP_LOGW(kDebugTag, "openTcp - already opened on port %d, ignored", aLocalPort);
		aErr = asio::error::already_open;
	} else {
		container.tcpListening.emplace_back(ioContext, asio::ip::tcp::endpoint{aTcp, aLocalPort});
		ESP_LOGI(kDebugTag, "openTcp - opened listening socket on port %d", aLocalPort);
		tcpAsyncAccept(container.tcpListening.back(), aLocalPort);
	}
}

void Api::tcpAsyncAccept(asio::ip::tcp::acceptor &aAcceptor, std::uint16_t aLocalPort)
{
	aAcceptor.async_accept(
		[this, &aAcceptor, aLocalPort] (asio::error_code aError, asio::ip::tcp::socket aSocket) mutable {
			std::lock_guard<std::mutex> lock{syncAsyncMutex};
			(void)lock;

			if (aError) {
				ESP_LOGE(kDebugTag, "tcpAsyncAccept error(%d)", aError.value());

				if (aError != asio::error::operation_aborted) {
					closeTcp(aLocalPort, aError);
				}
			} else {
				ESP_LOGI(kDebugTag, "tcpAsyncAccept accepted %s : %d on port %d",
					aSocket.remote_endpoint().address().to_string().c_str(), aSocket.remote_endpoint().port(),
					aLocalPort);
				container.tcpConnected.emplace_back(std::move(aSocket));
				tcpAsyncReceiveFrom(container.tcpConnected.back());
				tcpAsyncAccept(aAcceptor, aLocalPort);
			}
	});
}

void Api::openUdp(uint16_t aLocalPort, asio::error_code &aErr, asio::ip::udp aUdp)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.udp.find(aLocalPort);

	if (it != container.udp.end()) {
		aErr = asio::error::already_open;
		ESP_LOGW(kDebugTag, "openUdp on port %d - already opened", aLocalPort);
	} else {
		container.udp.emplace_back(ioContext, asio::ip::udp::endpoint{aUdp, aLocalPort});
		ESP_LOGI(kDebugTag, "openUdp on port %d - success", aLocalPort);
		udpAsyncReceiveFrom(container.udp.back());
	}
}

void Api::closeUdp(uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.udp.find(aPort);

	if (it == container.udp.end()) {
		ESP_LOGW(kDebugTag, "closeUdp on port %d - no such socket was found", aPort);
		aErr = asio::error::not_found;
	} else {
		it->close(aErr);
		container.udp.erase(it);
		ESP_LOGI(kDebugTag, "closeUdp on port %d - success", aPort);
	}
}

void Api::closeTcp(uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpListening.find(aPort);

	if (it == container.tcpListening.end()) {
		ESP_LOGW(kDebugTag, "closeTcp on port %d - no such socket was found", aPort);
		aErr = asio::error::not_found;
	} else {
		it->close(aErr);
		container.tcpListening.erase(it);
		ESP_LOGI(kDebugTag, "closeTcp on port %d - success", aPort);
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
			ESP_LOGI(kDebugTag, "udpAsyncReceiveFrom - received (%d bytes)", anReceived);
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
			udpAsyncReceiveFrom(aSocket);
		} else {
			ESP_LOGE(kDebugTag, "udpAsyncReceiveFrom on port %d - error(%d), closing", aSocket.local_endpoint().port(),
				aError.value());
			closeUdp(aSocket.local_endpoint().port(), aError);
		}
	});
}

void Api::tcpAsyncReceiveFrom(asio::ip::tcp::socket &aSocket)
{
	std::shared_ptr<char[]> buffer{new char[kReceiveBufferSize]};

	aSocket.async_receive(asio::buffer(buffer.get(), kReceiveBufferSize),
		[this, buffer, &aSocket](asio::error_code aErr, std::size_t anReceived) mutable {

		if (!aErr) {
			ESP_LOGI(kDebugTag, "udpAsyncReceiveFrom - received (%d bytes)", anReceived);
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
			tcpAsyncReceiveFrom(aSocket);
		} else if (Utility::Algorithm::in(aErr, asio::error::connection_reset, asio::error::eof,
			asio::error::bad_descriptor))
		{
			ESP_LOGE(kDebugTag, "tcpAsyncReceiveFrom %s : %d on port %d - error, disconnecting...",
				aSocket.remote_endpoint().address().to_string().c_str(), aSocket.remote_endpoint().port(),
				aSocket.local_endpoint().port());
			disconnect(aSocket.remote_endpoint(), aSocket.local_endpoint().port(), aErr);
		}
	});
}

}  // namespace Sock
