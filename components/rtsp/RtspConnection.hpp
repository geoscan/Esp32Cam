//
// RtspConnection.hpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTSPCONNECTION_HPP
#define COMPONENTS_RTSP_RTSPCONNECTION_HPP

#include "asio.hpp"
#include "RtspRequestHandler.hpp"
#include "RtpServer.hpp"

class RtspConnection final : public std::enable_shared_from_this<RtspConnection> {
public:
	RtspConnection(asio::ip::tcp::socket socket, RtpServer &rtp);
	void serve();
private:
	enum {
		kBufSize = 255,
	};

	asio::ip::tcp::tcp::socket tcpSocket;
	char buf[kBufSize];
	RtspRequestHandler requestHandler;
};

#endif // COMPONENTS_RTSP_RTSPCONNECTION_HPP
