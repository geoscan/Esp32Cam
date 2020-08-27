//
// Rtsp.hpp
//
// Created on:  Aug. 19, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTSPSERVER_HPP
#define COMPONENTS_RTSP_RTSPSERVER_HPP

#include "RtspConnection.hpp"
#include "RtpServer.hpp"
#include "Server.hpp"

// RTSP server implementation
// Invokes a creation of RTP streams over UDP

class RtspServer final : public Server {
public:
	RtspServer(asio::io_context &, unsigned port, RtpServer &);
	void run() override;

private:
	void acceptConnection();
	asio::ip::tcp::tcp::acceptor tcpAcceptor;
	RtpServer &rtpServer;
};

#endif // COMPONENTS_RTSP_RTSPSERVER_HPP
