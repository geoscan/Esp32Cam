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

RtspConnection::RtspConnection(tcp::socket socket)
	: tcpSocket(move(socket))
{
}

void RtspConnection::serve()
{
//	auto self(shared_from_this());
//	tcpSocket.async_read_some(asio::buffer(buf, kBufSize),
//		[this, self](std::error_code err, size_t length) {
//			if (!err) {
//				send(length);
//			}
//		});

	while (true) {
		asio::error_code err;
		size_t len = tcpSocket.receive(asio::buffer(buf, kBufSize), 0 /*no flags*/, err);
		Rtsp::Data data;

		if (!err) {
			data = requestHandler.handle(Rtsp::Data{static_cast<void *>(buf), len});
		} else {
			return;
		}

		if (data.len) {
			tcpSocket.send(asio::buffer(data.data, data.len));
		}
	}
}