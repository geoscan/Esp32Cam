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

class RtspRequestHandler {
public:
	/// @brief  Handles user's RTSP requests
	///         Prepares response
	/// @return Data to be sent to user
	const Rtsp::Data handle(const Rtsp::Data data);
private:
	const Rtsp::Data handlePlay(const Rtsp::Request &);
	const Rtsp::Data handleDescribe(const Rtsp::Request &);
	const Rtsp::Data handleSetup(const Rtsp::Request &);
	const Rtsp::Data handleTeardown(const Rtsp::Request &);
	const Rtsp::Data handlePause(const Rtsp::Request &);
	const Rtsp::Data handleNotStated(const Rtsp::Request &);
	enum : unsigned {
		kBufSize = 255,
	};
	char returnBuf[kBufSize] = {0};
};

#endif // COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP