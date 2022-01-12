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
	if (++instances == 1) {
		esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, CameraStreamTcpControl::handleApClientDisconnected, nullptr);
	}
}

CameraStreamTcpControl::~CameraStreamTcpControl()
{
	if (instances-- == 1) {  // last man standing
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, CameraStreamTcpControl::handleApClientDisconnected);
	}
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
	}
}

void CameraStreamTcpControl::handleApClientDisconnected(asio::ip::address)
{
	tcp.socket.close();
}

void CameraStreamTcpControl::handleApClientDisconnected(void */*nullptr*/, esp_event_base_t, int32_t, void *data)
{
	ip4_addr_t ipAddress;
	dhcp_search_ip_on_mac(reinterpret_cast<system_event_ap_stadisconnected_t *>(data)->mac, &ipAddress);

	Sub::Key::WifiDisconnected key;
	key.notify(asio::ip::address_v4(ntohl(ipAddress.addr)));
}