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

#include "utility/thr/Threading.hpp"
#include "camera_streamer.h"
#include "CameraStreamTcpControl.hpp"
#include "FrameSender.hpp"

using namespace CameraStreamer;

void cameraStreamerStart(asio::io_context &context)
{
	static asio::ip::tcp::socket tcpSocket(context);
	static asio::ip::tcp::acceptor acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), CONFIG_CAMSTREAM_SOURCE_TCP_PORT));
	static asio::ip::udp::socket udpSocket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), CONFIG_CAMSTREAM_SOURCE_UDP_PORT));

	static CameraStreamTcpControl cameraStreamTcpControl(acceptor, tcpSocket);
	static FrameSender frameSender(udpSocket);

	Utility::Thr::setThreadCoreAffinity(0);
	static std::thread threadCameraStreamTcpControl(&CameraStreamTcpControl::operator(), &cameraStreamTcpControl);
}