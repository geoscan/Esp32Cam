//
// RtspRequestHandler.hpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTSP_RTSPREQUESTHANDLER_HPP
#define COMPONENTS_RTSP_RTSP_RTSPREQUESTHANDLER_HPP

#include <cstddef>
#include <string>
#include <map>
#include "asio.hpp"
#include "Types.hpp"
#include "RtpServer.hpp"
#include "Media.hpp"
#include "ResponseComposer.hpp"

class RtspRequestHandler {
public:
	RtspRequestHandler(RtpServer &rtp) : rtpServer(rtp)
	{
	}

	/// @brief  Handles user's RTSP requests
	///         Prepares response
	/// @return Data to be sent to user
	std::string handle(asio::const_buffer buffer, asio::ip::address from);
private:
	std::string handlePlay(const Rtsp::Request &);
	std::string handleDescribe(const Rtsp::Request &);
	std::string handleOptions(const Rtsp::Request &);
	std::string handleSetup(const Rtsp::Request &);
	std::string handleTeardown(const Rtsp::Request &);
	std::string handlePause(const Rtsp::Request &);
	std::string handleNotStated(const Rtsp::Request &);

	RtpServer &rtpServer;
	Rtsp::Media media;
};

#endif // COMPONENTS_RTSP_RTSP_RTSPREQUESTHANDLER_HPP