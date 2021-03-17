#include "FrameSender.hpp"

using namespace CameraStreamer;

FrameSender::FrameSender(asio::ip::udp::socket &aSocket) :
	socket(aSocket),
	key {{&FrameSender::processFrame, this},
		{&FrameSender::processTcpConnected, this},
		{&FrameSender::processTcpDisconnected, this}}
{
}

void FrameSender::processFrame(std::shared_ptr<Ov2640::Image> img)
{
	asio::error_code err;
	for (auto &client : clients) {
		if (client.enabled) {
			socket.send_to(img->data(), client.endpoint, 0, err);
		}
	}
}

void FrameSender::processTcpConnected(asio::ip::address addr, unsigned short port)
{
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
	for (auto &client : clients) {
		if (client.endpoint.address() == addr) {
			client.enabled = false;
		}
	}
}