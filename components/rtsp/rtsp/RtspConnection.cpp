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
	requestHandler(rtp)
{
}

void RtspConnection::serve()
{
	auto self(shared_from_this());
	tcpSocket.async_read_some(asio::buffer(buf, kBufferSize),
		[this, self](std::error_code err, size_t length) {
			if (!err && length >= kMinRtspRequestLength) {
				std::string respBuffer;
				respBuffer = requestHandler.handle(asio::buffer(buf, length), tcpSocket.remote_endpoint().address());
				if (respBuffer.empty()) {
					std::vector<asio::const_buffer> vec = {{respBuffer.c_str(), respBuffer.length()}};
					tcpSocket.send(vec);
				}
			}
			serve();
		});
//	asio::error_code err;
//	size_t length = tcpSocket.read_some(asio::buffer(buf, kBufferSize), err);
//	if (!err && length >= kMinRtspRequestLength) {
//		std::string respBuffer ;
//		respBuffer = requestHandler.handle(asio::buffer(asio::buffer(buf, length)), tcpSocket.remote_endpoint().address());
//		if (!respBuffer.empty()) {
//			std::vector<asio::const_buffer> vec = {{respBuffer.c_str(), respBuffer.length()}};
//			tcpSocket.send(vec);
//		}
//	}
}