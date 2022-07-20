//
// Api.cpp
//
// Created on: Feb 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <sdkconfig.h>
// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_SOCKET_DEBUG_LEVEL)
#include <esp_log.h>
#include "utility/LogSection.hpp"

#include "socket/socket.hpp"
#include "socket/Api.hpp"
#include "sub/Rout.hpp"
#include "utility/Algorithm.hpp"
#include "utility/thr/WorkQueue.hpp"

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

		{
			std::error_code err;
			auto localEndpoint = it->local_endpoint(err);

			if (!err) {
				aLocalPort = localEndpoint.port();
			}
		}

		ESP_LOGW(kDebugTag, "connect to %s : %d already connected from port %d",
			aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aLocalPort);
	} else {
		ESP_LOGD(Sock::kDebugTag, "Api::connect(TCP): opening socket");
		container.tcpConnected.emplace_back(ioContext);
		container.tcpConnected.back().open(aTcp, aErr);

		ESP_LOGD(Sock::kDebugTag, "Api::connect(TCP): binding socket");
		if (!aErr && 0 != aLocalPort) {
			container.tcpConnected.back().bind(asio::ip::tcp::endpoint{aTcp, aLocalPort}, aErr);
		}

		ESP_LOGD(kDebugTag, "Api::connect(TCP): connecting to %s : %d from port %d", aRemoteEndpoint.address().to_string().c_str(),
			aRemoteEndpoint.port(), aLocalPort);
		if (!aErr) {
			container.tcpConnected.back().connect(aRemoteEndpoint, aErr);
		}

		if (aErr) {
			asio::error_code errTemp;
			ESP_LOGE(kDebugTag, "connect to %s : %d from port %d - error(%d)",
				aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aLocalPort, aErr.value());
			container.tcpConnected.back().shutdown(asio::ip::tcp::socket::shutdown_type::shutdown_both, errTemp);
			container.tcpConnected.back().close(errTemp);
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
		container.tcpListening.emplace_back(ioContext);
		container.tcpListening.back().open(aTcp, aErr);

		if (!aErr) {
			container.tcpListening.back().bind(asio::ip::tcp::endpoint{aTcp, aLocalPort}, aErr);
		}

		if (!aErr) {
			static constexpr auto kBacklogSize = 4;
			container.tcpListening.back().listen(kBacklogSize, aErr);
		}

		if (!aErr) {
			ESP_LOGI(kDebugTag, "openTcp - opened listening socket on port %d", aLocalPort);
			tcpAsyncAccept(container.tcpListening.back(), aLocalPort);
		} else {
			asio::error_code errTemp;
			ESP_LOGE(kDebugTag, "openTcp - open listening socket on port %d - error(%d)", aLocalPort, aErr.value());
			container.tcpListening.back().close(errTemp);
		}
	}
}

void Api::tcpAsyncAccept(asio::ip::tcp::acceptor &aAcceptor, std::uint16_t aLocalPort)
{
	using namespace Utility::Thr;
	aAcceptor.async_accept(
		[this, &aAcceptor, aLocalPort] (asio::error_code aError, asio::ip::tcp::socket aSocket) mutable {
			if (aError) {
				ESP_LOGE(kDebugTag, "tcpAsyncAccept error(%d)", aError.value());

				if (aError != asio::error::operation_aborted) {
				 	std::lock_guard<std::mutex> lock{syncAsyncMutex};
					(void)lock;

					closeTcp(aLocalPort, aError);
				}
			} else {
				std::error_code err;
				auto epRemote = aSocket.remote_endpoint(err);
				ESP_LOGI(kDebugTag, "tcpAsyncAccept accepted %s : %d on port %d",
					epRemote.address().to_string().c_str(), epRemote.port(),
					aLocalPort);
				Wq::MediumPriority::getInstance().push(
					[&epRemote, &aLocalPort]()
					{
						Sub::Rout::OnTcpEvent::notify(Sub::Rout::TcpConnected{epRemote, aLocalPort});
					});
				std::lock_guard<std::mutex> lock{syncAsyncMutex};
				(void)lock;
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
		container.udp.emplace_back(ioContext);
		container.udp.back().open(aUdp, aErr);

		if (!aErr) {
			container.udp.back().bind(asio::ip::udp::endpoint{aUdp, aLocalPort}, aErr);
		}

		if (!aErr) {
			ESP_LOGI(kDebugTag, "openUdp on port %d - success", aLocalPort);
			udpAsyncReceiveFrom(container.udp.back());
		} else {
			asio::error_code errTemp;
			ESP_LOGE(kDebugTag, "openUdp on port %d - error(%d)", aLocalPort, aErr.value());
			container.udp.back().shutdown(asio::ip::udp::socket::shutdown_type::shutdown_both, errTemp);
			container.udp.back().close(errTemp);
		}
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
		it->shutdown(asio::ip::udp::socket::shutdown_type::shutdown_both, aErr);
		it->close(aErr);
		container.udp.erase(it);
		ESP_LOGI(kDebugTag, "closeUdp on port %d - success", aPort);
	}
}

void Api::closeTcp(uint16_t aPort, asio::error_code &aErr)
{
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void) lock;
#if defined(CONFIG_SOCKET_SERVER_CLOSE_CLI_DISCONNECT)
	{
		asio::error_code err;
		ESP_LOGI(kDebugTag, "closeTcp on port %d - disconnecting clients", aPort);
		for (auto it = container.tcpConnected.begin(); container.tcpConnected.end() != it;) {
			auto ep = it->local_endpoint(err);

			if (!err && ep.port() == aPort) {
				it->shutdown(asio::ip::tcp::socket::shutdown_type::shutdown_both, err);
				it->close();
				it = container.tcpConnected.erase(it);
			} else {
				++it;
			}

			err.clear();
		}
	}
#endif  // defined(CONFIG_SOCKET_SERVER_CLOSE_CLI_DISCONNECT)

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
	using namespace Utility::Thr;
	std::shared_ptr<char[]> buffer {new char[kReceiveBufferSize]};
	auto endpoint = std::make_shared<asio::ip::udp::endpoint>();
	auto port = aSocket.local_endpoint().port();

	aSocket.async_receive_from(asio::buffer(buffer.get(), kReceiveBufferSize), *endpoint.get(),
		[this, buffer, endpoint, port, &aSocket] (asio::error_code aError, std::size_t anReceived) mutable {

		if (!aError) {
			ESP_LOGV(kDebugTag, "udpAsyncReceiveFrom - received (%d bytes)", anReceived);
			for (auto &cb : Sub::Rout::OnReceived::getIterators()) { // Notify subscribers

				// Reduce memory expenses on stack memory allocation
				Wq::MediumPriority::getInstance().pushWait(
					[this, &cb, &endpoint, &port, &buffer, &anReceived, &aSocket]() mutable
					{
						auto response = cb({Sub::Rout::Socket<asio::ip::udp>{*endpoint.get(), port,
							asio::const_buffer(buffer.get(), anReceived)}});

						// If a subscriber provides a response, send it
						if (response.getType() == Sub::Rout::Response::Type::Response) {
							ESP_LOGV(kDebugTag, "udpAsyncReceiveFrom - sending respose (%d bytes)",
								response.payload.size());
							std::lock_guard<std::mutex> lock{syncAsyncMutex};
							(void)lock;
							asio::error_code err;
							aSocket.send_to(response.payload, *endpoint.get(), 0, err);
						}
					});
			}
			ESP_LOGV(kDebugTag, "udpAsyncReceiveFrom - next round");
			udpAsyncReceiveFrom(aSocket);
		} else {
			ESP_LOGE(kDebugTag, "udpAsyncReceiveFrom on port %d - error(%d), closing", port,
				aError.value());

			if (aError != asio::error::operation_aborted) {
				closeUdp(aSocket.local_endpoint().port(), aError);
			}
		}
	});
}

void Api::tcpAsyncReceiveFrom(asio::ip::tcp::socket &aSocket)
{
	using namespace Utility::Thr;
	std::shared_ptr<char[]> buffer{new char[kReceiveBufferSize]};

	aSocket.async_receive(asio::buffer(buffer.get(), kReceiveBufferSize),
		[this, buffer, &aSocket](asio::error_code aErr, std::size_t anReceived) mutable {

		if (!aErr) {
			ESP_LOGV(kDebugTag, "tcpAsyncReceiveFrom - received (%d bytes)", anReceived);
			std::error_code err;
			auto epRemote = aSocket.remote_endpoint(err);
			typename Sub::Rout::OnReceived::Ret response;

			ESP_LOGV(kDebugTag, "tcpAsyncReceiveFrom - notifying subscribers");
			for (auto &cb : Sub::Rout::OnReceived::getIterators()) {  // Notify subscribers

				for (auto bufView = Utility::toBuffer<const std::uint8_t>(buffer.get(), anReceived);
					bufView.size();
					bufView = response.nProcessed > 0 && response.nProcessed < bufView.size() ?
					bufView.slice(response.nProcessed) :
					bufView.slice(bufView.size()))
				{

					ESP_LOGV(kDebugTag, "tcpAsyncReceiveFrom(): processing (%d bytes remain)", bufView.size());

					// Reduce expenses on stack memory allocation
					Wq::MediumPriority::getInstance().pushWait(
						[&response, &epRemote, &aSocket, &anReceived, &buffer, &cb]()
						{
							response = cb({Sub::Rout::Socket<asio::ip::tcp>{epRemote, aSocket.local_endpoint().port(),
								asio::const_buffer(buffer.get(), anReceived) }});
						});

					ESP_LOGV(kDebugTag, "tcpAsyncReceiveFrom(): chunk nProcessed %d", response.nProcessed);

					// If a subscriber provides a response, send it
					if (response.getType() == Sub::Rout::Response::Type::Response) {
						ESP_LOGV(kDebugTag, "tcpAsyncReceiveFrom - got response (%d bytes) sending", response.payload.size());
						asio::error_code err;
						aSocket.write_some(response.payload, err);
					}

					response.reset();
				}
			}
			tcpAsyncReceiveFrom(aSocket);
		} else if (Utility::Algorithm::in(aErr, asio::error::connection_reset, asio::error::eof,
			asio::error::bad_descriptor))
		{
			asio::error_code err;
			auto epRemote = aSocket.remote_endpoint(err);
			std::uint16_t portLocal = 0;

			if (!err) {
				auto epLocal = aSocket.local_endpoint(err);

				if (!err) {
					portLocal = epLocal.port();
				}
			}

			if (!err) {
				ESP_LOGE(kDebugTag, "tcpAsyncReceiveFrom %s : %d on port %d - error(%d), disconnecting...",
					epRemote.address().to_string().c_str(), epRemote.port(), portLocal, aErr.value());
				Wq::MediumPriority::getInstance().push(
					[&epRemote, portLocal]()
					{
						Sub::Rout::OnTcpEvent::notify(Sub::Rout::TcpDisconnected{epRemote, portLocal});
					});
				disconnect(epRemote, portLocal, aErr);
			} else {
				ESP_LOGE(kDebugTag, "tcpAsyncReceiveFrom - disconnect failure. Could not get the client's endpoint");
			}
		}
	});
}

///
/// \brief            Send a TCP packet
/// \param aEndpoint  Remote endpoint
/// \param aLocalPort If 0, an attempt to reuse some existing connection with the specified endpoitn will be made.
/// \param aBuffer    Data to send
/// \param aErr       Retured error code
/// \return           Number of bytes sent
///
std::size_t Api::sendTo(const asio::ip::tcp::endpoint &aEndpoint, std::uint16_t &aLocalPort, asio::const_buffer aBuffer,
	asio::error_code &aErr)
{
	GS_UTILITY_LOG_SECTIONV(kDebugTag, "Api::sendTo(TCP)");
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it = container.tcpConnected.end();

	if (0 == aLocalPort) {
		it = container.tcpConnected.find(aEndpoint);
	} else {
		it = container.tcpConnected.find(aEndpoint, aLocalPort);
	}

	if (it == container.tcpConnected.end()) {
		ESP_LOGW(kDebugTag, "sendTo(TCP) %s : %d on port %d - no such socket was found",
			aEndpoint.address().to_string().c_str(), aEndpoint.port(), aLocalPort);
		return asio::error::not_connected;
	}

	return it->write_some(aBuffer, aErr);
}

std::size_t Api::sendTo(const asio::ip::udp::endpoint &aRemoteEndpoint, std::uint16_t &aLocalPort, asio::const_buffer aBuffer,
	asio::error_code &aErr, asio::ip::udp aUdp)
{
	GS_UTILITY_LOG_SECTIONV(kDebugTag, "Api::sendTo(UDP)");
	std::lock_guard<std::mutex> lock{syncAsyncMutex};
	(void)lock;
	auto it{container.udp.end()};
	aErr.clear();

	if (0 != aLocalPort) {
		it = container.udp.find(aLocalPort);
	}

	if (it == container.udp.end()) {
		it = container.udp.emplace(container.udp.end(), ioContext);

		it->open(aUdp, aErr);

		if (!aErr) {
			it->bind(asio::ip::udp::endpoint{aUdp, aLocalPort}, aErr);
		}

		if (!aErr) {
			aLocalPort = it->local_endpoint(aErr).port();
		}

		if (!aErr) {
			ESP_LOGI(kDebugTag, "sendTo(UDP) %s : %d - no such socket was found, created on port %d",
				aRemoteEndpoint.address().to_string().c_str(), aRemoteEndpoint.port(), aLocalPort);
			udpAsyncReceiveFrom(*it);
		}
	}

	std::size_t ret = 0;

	if (container.udp.end() != it) {
		ret = it->send_to(aBuffer, aRemoteEndpoint, 0, aErr);
	}

	return ret;
}

}  // namespace Sock
