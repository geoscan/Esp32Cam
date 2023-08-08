//
// CameraStreamTcpControl.cpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

extern "C" {
#include <dhcpserver/dhcpserver.h>
#include <esp_wifi.h>
#include <esp_log.h>
}
#include <esp_event.h>
#include <list>
#include <mutex>
#include "CameraStreamTcpControl.hpp"

using namespace CameraStreamer;

size_t CameraStreamTcpControl::instances = 0;

CameraStreamTcpControl::CameraStreamTcpControl(asio::ip::tcp::acceptor &tcpAcceptor, asio::ip::tcp::socket &tcpSocket) :
	tcp{tcpSocket, tcpAcceptor},
	key{{}, {}, {&CameraStreamTcpControl::handleApClientDisconnected, this}}
{
}

CameraStreamTcpControl::~CameraStreamTcpControl()
{
}

void CameraStreamTcpControl::operator()()
{
	while (true) {
		asio::ip::tcp::endpoint clientEndpoint;
		asio::error_code        err;

		tcp.acceptor.accept(tcp.socket, clientEndpoint, err);
		if (!err) {
			key.tcpConnected.notify(clientEndpoint.address(), clientEndpoint.port());

			{
				auto str = clientEndpoint.address().to_string();
				auto port = clientEndpoint.port();

				ESP_LOGI("[camera_streamer]", "connected, TCP %s:%d", str.c_str(), port);
			}

			char stubBuffer[1];
			std::error_code err;

			while (err != asio::error::connection_reset && err != asio::error::eof && err != asio::error::bad_descriptor) {
				tcp.socket.receive(asio::buffer(stubBuffer, 1), 0, err);
			}
			tcp.socket.close();

			key.tcpDisconnected.notify(clientEndpoint.address());

			{
				auto str = clientEndpoint.address().to_string();
				auto port = clientEndpoint.port();

				ESP_LOGI("[camera_streamer]", "disconnected, TCP %s:%d", str.c_str(), port);
			}
		}

		vTaskDelay(1);
	}
}

void CameraStreamTcpControl::handleApClientDisconnected(asio::ip::address)
{
	tcp.socket.close();
}
