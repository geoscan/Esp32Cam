//
// FrameSender.hpp
//
// Created on: Mar 17, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "FrameSender.hpp"
#include <esp_log.h>

using namespace CameraStreamer;

FrameSender::FrameSender(asio::ip::udp::socket &aSocket) :
	socket(aSocket),
	key {{&FrameSender::processFrame, this},
		{&FrameSender::processTcpConnected, this},
		{&FrameSender::processTcpDisconnected, this}}
{
}

void FrameSender::processFrame(const std::shared_ptr<Ov2640::Image> &img)
{
	asio::error_code err;
	for (auto &client : clients) {
		if (client.enabled) {
			socket.send_to(img.get()->data(), client.endpoint, 0, err);
		}
	}
}

void FrameSender::processTcpConnected(asio::ip::address addr, unsigned short port)
{
	{
		auto ip = addr.to_string();
		ESP_LOGI("[camera_streamer]", "stream start, client %s:%d", ip.c_str(), port);
	}
	asio::ip::udp::endpoint endpoint{addr, port};
	for (auto &client : clients) {
		if (client.endpoint == endpoint) {
			client.endpoint.port(port);
			client.enabled = true;
			return;
		}
	}
	clients.emplace_front(Client{endpoint, true});
}

void FrameSender::processTcpDisconnected(asio::ip::address addr)
{
	{
		auto ip = addr.to_string();
		ESP_LOGI("[camera_streamer]", "stream stop, client %s", ip.c_str());
	}
	for (auto &client : clients) {
		if (client.endpoint.address() == addr) {
			client.enabled = false;
		}
	}
}