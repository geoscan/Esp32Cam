//
// RtspRequestHandler.hpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTSP_RTSPREQUESTHANDLER_HPP
#define COMPONENTS_RTSP_RTSP_RTSPREQUESTHANDLER_HPP

#include <cstddef>
#include "asio.hpp"
#include "Types.hpp"
#include "RtpServer.hpp"
#include <string>


class RtspRequestHandler {
public:
	RtspRequestHandler(RtpServer &rtp) : rtpServer(rtp)
	{
	}

	/// @brief  Handles user's RTSP requests
	///         Prepares response
	/// @return Data to be sent to user
	std::string handle(asio::const_buffer buffer);
private:
	std::string handlePlay(const Rtsp::Request &);
	std::string handleDescribe(const Rtsp::Request &);
	std::string handleOption(const Rtsp::Request &);
	std::string handleSetup(const Rtsp::Request &);
	std::string handleTeardown(const Rtsp::Request &);
	std::string handlePause(const Rtsp::Request &);
	std::string handleNotStated(const Rtsp::Request &);

	static constexpr const char *kRtspVer = "RTSP/1.0";
	static constexpr const char *kCrlf = "\r\n";
	static constexpr const char *kCseq = "CSeq:";
	static std::string          dateHeader();

	enum : unsigned {
		kBufSize = 255,
	};

	char returnBuf[kBufSize] = {0};
	RtpServer &rtpServer;
};

#endif // COMPONENTS_RTSP_RTSP_RTSPREQUESTHANDLER_HPP