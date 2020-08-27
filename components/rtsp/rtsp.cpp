//
// rtsp.cpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "asio.hpp"
#include "RtspServer.hpp"
#include "rtsp.h"
#include "esp_pthread.h"
#include "pthread.h"
#include "Server.hpp"

/// Expects 'Runnable' having method 'run()'
template <typename Runnable>
static void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

void rtspStart()
{
	static asio::io_context rtpContext;
	static asio::io_context rtspContext;

	static RtpServer rtpServer(rtpContext, kRtpPort);
	static RtspServer rtspServer(rtspContext, kRtspPort, rtpServer);

	esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
	// XXX: alter default config if needed
	esp_pthread_set_cfg(&cfg);

	pthread_t stub;
	pthread_create(&stub, NULL, run<RtpServer>, reinterpret_cast<void *>(&rtpServer));
	pthread_create(&stub, NULL, run<RtspServer>, reinterpret_cast<void *>(&rtspServer));
	pthread_create(&stub, NULL, run<asio::io_context>, reinterpret_cast<void *>(&rtpContext));
	pthread_create(&stub, NULL, run<asio::io_context>, reinterpret_cast<void *>(&rtspContext));
}