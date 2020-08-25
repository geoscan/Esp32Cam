//
// RtspRequestHandler.hpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP
#define COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP

#include <cstddef>
#include "asio.hpp"
#include "Types.hpp"
#include "RtpServer.hpp"

class RtspRequestHandler {
public:
	RtspRequestHandler(RtpServer &rtp) : rtpServer(rtp)
	{
	}

	/// @brief  Handles user's RTSP requests
	///         Prepares response
	/// @return Data to be sent to user
	asio::const_buffer handle(asio::const_buffer buffer);
private:
	asio::const_buffer handlePlay(const Rtsp::Request &);
	asio::const_buffer handleDescribe(const Rtsp::Request &);
	asio::const_buffer handleSetup(const Rtsp::Request &);
	asio::const_buffer handleTeardown(const Rtsp::Request &);
	asio::const_buffer handlePause(const Rtsp::Request &);
	asio::const_buffer handleNotStated(const Rtsp::Request &);

	enum : unsigned {
		kBufSize = 255,
	};

	char returnBuf[kBufSize] = {0};
	RtpServer &rtpServer;
};

#endif // COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP