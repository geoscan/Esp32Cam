#ifndef COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP
#define COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP

#include <cstddef>
#include "asio.hpp"
#include "Types.hpp"

class RtspRequestHandler {
public:
	/// @brief  Handles user's RTSP requests
	/// @return Data to be sent to user
	Rtsp::Data handle(Rtsp::Data data);
};

#endif // COMPONENTS_RTSP_RTSPREQUESTHANDLER_HPP