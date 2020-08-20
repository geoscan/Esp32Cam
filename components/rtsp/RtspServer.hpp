//
// Rtsp.hpp
//
// Created on:  Aug. 19, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTSPSERVER_HPP
#define COMPONENTS_RTSP_RTSPSERVER_HPP

#include "RtspConnection.hpp"

// RTSP server implementation
// Invokes a creation of RTP streams over UDP

class RtspServer final {
public:
	RtspServer(asio::io_context &);
	void acceptConnection();

private:
	enum {
		kPort = 554, // Standard RTSP port
	};

	asio::ip::tcp::tcp::acceptor tcpAcceptor;
};

#endif // COMPONENTS_RTSP_RTSPSERVER_HPP
