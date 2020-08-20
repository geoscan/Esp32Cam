#ifndef COMPONENTS_RTSP_RTSPCONNECTION_HPP
#define COMPONENTS_RTSP_RTSPCONNECTION_HPP

#include "asio.hpp"

class RtspConnection final : public std::enable_shared_from_this<RtspConnection> {
public:
	RtspConnection(asio::ip::tcp::socket socket);
	void start();
private:
	enum {
		kBufSize = 255,
	};
	void receive();
	void send(size_t len);

	asio::ip::tcp::tcp::socket tcpSocket;
	char buf[kBufSize];
};

#endif // COMPONENTS_RTSP_RTSPCONNECTION_HPP
