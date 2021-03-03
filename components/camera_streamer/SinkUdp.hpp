//
// SinkUdp.hpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// Encapsulates clients' IP/UDP addresses/ports, receives Ov2640's frames from
// CameraStream and transfers them over UDP to the clients.
//

#ifndef CAMERA_STREAMER_SINKUDP_HPP
#define CAMERA_STREAMER_SINKUDP_HPP

#include <asio.hpp>
#include "utility/Subscription.hpp"
#include "Ov2640.hpp"

class SinkUdp : public Utility::Subscription::Subscriber {
public:
	SinkUdp(asio::ip::udp::socket &, asio::ip::address, unsigned short port);
	void process(void *image) override;
protected:
	asio::ip::udp::endpoint endpoint;
	asio::ip::udp::socket   &socket;
};

#endif  // CAMERASTREAMER_SINKUDP_HPP
