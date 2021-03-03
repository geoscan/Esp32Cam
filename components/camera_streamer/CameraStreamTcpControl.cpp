//
// CameraStreamTcpControl.cpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//


extern "C" {
#include <dhcpserver/dhcpserver.h>
#include <esp_wifi.h>
}
#include <esp_event.h>


#include <list>
#include <mutex>

#include "CameraStreamTcpControl.hpp"

// Static storage of Camera..Control's instances. Necessary for proper
// management of subscription on WIFI_EVENT_AP_STADISCONNECTED event.

static struct {
	std::set<CameraStreamTcpControl *> instances;
	std::mutex mutex;
} controls;


CameraStreamTcpControl::CameraStreamTcpControl(CameraStream &aCameraStream,
	asio::ip::tcp::acceptor &tcpAcceptor,
	asio::ip::tcp::socket &tcpSocket,
	asio::ip::udp::socket &udpSocket) :
	cameraStream(aCameraStream),
	tcp{tcpSocket, tcpAcceptor},
	sinks{{}, {}, udpSocket}
{
	std::lock_guard<std::mutex> lock(controls.mutex);
	if (controls.instances.empty()) {
		esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED,
			CameraStreamTcpControl::handleApClientDisconnected, nullptr);
	}
	controls.instances.insert(this);
}

CameraStreamTcpControl::~CameraStreamTcpControl()
{
	std::lock_guard<std::mutex> lock(controls.mutex);
	controls.instances.erase(controls.instances.find(this));
	if (controls.instances.empty()) {
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED,
			CameraStreamTcpControl::handleApClientDisconnected);
	}
}

void CameraStreamTcpControl::operator()()
{
	while (true) {
		asio::ip::tcp::endpoint clientEndpoint;
		asio::error_code        err;

		tcp.acceptor.accept(tcp.socket, clientEndpoint, err);
		if (!err) {
			addUdpSink(clientEndpoint.address(), clientEndpoint.port());

			char stubBuffer[1];
			std::error_code err;

			while (err != asio::error::connection_reset && err != asio::error::eof && err != asio::error::bad_descriptor) {
				tcp.socket.receive(asio::buffer(stubBuffer, 1), 0, err);
			}

			removeUdpSink(clientEndpoint.address());
			tcp.socket.close();
		}
	}
}

void CameraStreamTcpControl::addUdpSink(asio::ip::address addr, unsigned short port)
{
	std::lock_guard<std::mutex> lock(sinks.mutex);
	auto it = sinks.map.insert({addr, {sinks.udpSocket, addr, port}});
	cameraStream.addSubscriber(it.first->second);
}

void CameraStreamTcpControl::removeUdpSink(asio::ip::address addr)
{
	std::lock_guard<std::mutex> lock(sinks.mutex);
	auto it = sinks.map.find(addr);

	if (it != sinks.map.end()) {
		cameraStream.removeSubscriber(it->second);
		sinks.map.erase(it);
	}
}

void CameraStreamTcpControl::handleApClientDisconnected(asio::ip::address addr)
{
	removeUdpSink(addr);
	tcp.socket.close();
}

void CameraStreamTcpControl::handleApClientDisconnected(void */*nullptr*/, esp_event_base_t, int32_t, void *data)
{
	std::lock_guard<std::mutex> lock(controls.mutex);
	for (auto instancePtr : controls.instances) {
		auto *eventData = reinterpret_cast<system_event_ap_stadisconnected_t *>(data);
		ip4_addr_t ipAddress;

		dhcp_search_ip_on_mac(eventData->mac, &ipAddress);
		instancePtr->handleApClientDisconnected(asio::ip::address_v4(ntohl(ipAddress.addr)));
	}
}