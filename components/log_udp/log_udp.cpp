//
// log_udp.cpp
//
// Created on: Mar 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

extern "C" {
#include <esp_log.h>
}
#include "private_include/Endpoint.hpp"
#include "log_udp.h"
#include <sdkconfig.h>

static LogUdp::Endpoint *endpoint = nullptr;

extern "C" int debugFprint(const char *format, va_list args)
{
	if (!endpoint) {
		return -1;
	}

	char buf[CONFIG_DEBUG_UDP_MSGLEN] = {0};
	vsnprintf(buf, CONFIG_DEBUG_UDP_MSGLEN, format, args);

	buf[CONFIG_DEBUG_UDP_MSGLEN - 1] = 0;
	endpoint->send(buf);

	return 0;
}

void logUdpStart(asio::io_context &ioContext)
{
#if !CONFIG_DEBUG_UDP_ENABLE
	return;
#endif
	// Init endpoint
	static asio::ip::udp::socket socket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), CONFIG_DEBUG_UDP_PORT));
	static LogUdp::Endpoint ep(socket);
	static std::thread thread(&LogUdp::Endpoint::operator(), &ep);

	// Redirect logging
	endpoint = &ep;
	esp_log_set_vprintf(debugFprint);
}