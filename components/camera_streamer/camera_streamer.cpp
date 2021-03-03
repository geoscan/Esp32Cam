//
// camera_streamer.cpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <memory>
#include <thread>

#include "utility/Run.hpp"
#include "camera_streamer.h"
#include "CameraStream.hpp"
#include "CameraStreamTcpControl.hpp"

#ifndef CONFIG_CAMSTREAM_FPS
# define CONFIG_CAMSTREAM_FPS -1
#endif  // CONFIG_CAMSTREAM_FPS

template <typename Runnable>
static void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

void cameraStreamerStart(asio::io_context &context)
{
	static asio::ip::tcp::socket tcpSocket(context);
	static asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), CONFIG_CAMSTREAM_SOURCE_TCP_PORT));
	static asio::ip::udp::socket udpSocket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), CONFIG_CAMSTREAM_SOURCE_UDP_PORT));

	static CameraStream cameraStream(CONFIG_CAMSTREAM_FPS);
	static CameraStreamTcpControl cameraStreamTcpControl(cameraStream, acceptor, tcpSocket, udpSocket);

	Utility::setThreadCoreAffinity(1);
	static std::thread threadCameraStream(&CameraStream::operator(), &cameraStream);

	Utility::setThreadCoreAffinity(0);
	static std::thread threadCameraStreamTcpControl(&CameraStreamTcpControl::operator(), &cameraStreamTcpControl);
}