//
// FrameSender.hpp
//
// Created on: Mar 17, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_STREAMER_FRAMESENDER_HPP
#define CAMERA_STREAMER_FRAMESENDER_HPP

#include <forward_list>
#include "Ov2640.hpp"
#include "Messaging.hpp"

namespace CameraStreamer {

class FrameSender {
public:
	FrameSender(asio::ip::udp::socket &);
	void processFrame(std::shared_ptr<Ov2640::Image>);
	void processTcpConnected(asio::ip::address, unsigned short);
	void processTcpDisconnected(asio::ip::address);
private:
	asio::ip::udp::socket &socket;
	struct {
		Key::NewFrame        newFrame;
		Key::TcpConnected    tcpConnected;
		Key::TcpDisconnected tcpDisconnected;
	} key;
	struct Client {
		asio::ip::udp::endpoint endpoint;
		bool enabled;
	};
	std::forward_list<Client> clients;
};

}  // namespace CameraStreamer

#endif  // CAMERA_STREAMER_FRAMESENDER_HPP
