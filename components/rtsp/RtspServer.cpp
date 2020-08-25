//
// RtspServer.cpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "RtspServer.hpp"
#include <memory>

using namespace std;
using asio::ip::tcp;

RtspServer::RtspServer(asio::io_context &context, unsigned port, RtpServer &rtp) :
	tcpAcceptor(context, tcp::endpoint(tcp::v4(), port)),
	rtpServer(rtp)
{
	acceptConnection();
}

void RtspServer::acceptConnection()
{
	tcpAcceptor.async_accept(
		[this](error_code err, tcp::socket socket) {
			if (!err) {
				make_shared<RtspConnection>(std::move(socket), rtpServer)->serve();
			}
			acceptConnection();
		});
}