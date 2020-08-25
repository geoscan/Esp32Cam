//
// RtspConnection.cpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "RtspConnection.hpp"
#include <algorithm>
#include "Types.hpp"

using namespace std;
using namespace asio::ip;

RtspConnection::RtspConnection(tcp::socket socket, RtpServer &rtp) :
	tcpSocket(move(socket)),
	rtpServer(rtp)
{
}

void RtspConnection::serve()
{
	auto self(shared_from_this());
	tcpSocket.async_read_some(asio::buffer(buf, kBufSize),
		[this, self](std::error_code err, size_t length) {
			if (!err) {
				asio::const_buffer buf = requestHandler.handle({buf, length});
				tcpSocket.send(buf, 0);
			}
			serve();
		});
}