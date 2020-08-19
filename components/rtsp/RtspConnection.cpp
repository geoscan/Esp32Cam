#include "RtspConnection.hpp"
#include <algorithm>

using namespace std;
using namespace asio::ip;

RtspConnection::RtspConnection(tcp::socket socket)
	: tcpSocket(move(socket))
{
	start();
}

void RtspConnection::start()
{
	receive();
}

void RtspConnection::receive()
{
	auto self(shared_from_this());
	tcpSocket.async_read_some(asio::buffer(buf, kBufSize),
		[this, self](std::error_code err, size_t length) {
			if (!err) {
				// TODO process message
			}
		});
}

void RtspConnection::send()
{
	// TODO
}