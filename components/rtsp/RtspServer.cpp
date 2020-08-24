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

RtspServer::RtspServer(asio::io_context &context) :
	tcpAcceptor(context, tcp::endpoint(tcp::v4(), RtspServer::kPort))
{
	acceptConnection();
}

void RtspServer::acceptConnection()
{
	// FIXME: the fact that we only communicate over one network
	// interface eliminates the need to have an asynchronous user
	// request processing. Make it single-threaded/
	tcpAcceptor.async_accept(
		[this](error_code err, tcp::socket socket) {
			if (!err) {
				make_shared<RtspConnection>(std::move(socket))->start();
			}
			acceptConnection();
		});
}