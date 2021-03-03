//
// SinkUdp.cpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "SinkUdp.hpp"

SinkUdp::SinkUdp(asio::ip::udp::socket &sock, asio::ip::address addr, unsigned short port) :
	endpoint(addr, port), socket(sock)
{
}

void SinkUdp::process(void *image)
{
	auto img = reinterpret_cast<std::shared_ptr<Ov2640::Image> *>(image);
	asio::error_code err;
	socket.send_to((*img)->data(), endpoint, 0, err);
}