//
// CameraStreamer.hpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERASTREAMER_HPP
#define CAMERASTREAMER_HPP

#include "asio.hpp"

class CameraStreamer final {
public:
	using Fps = short;
	/// @param fps <= 0 -- Instant frame sending
	///            >  0 -- Use provided value as max FPS
	CameraStreamer(asio::io_context &context, uint16_t sourcePort /*this port*/,
		uint16_t sinkPort, Fps fps = -1);
	void run();
private:
	static uint32_t currentTimeMs();

	asio::ip::udp::socket   socket;
	asio::ip::udp::endpoint sink;
	const Fps               fps;
};

#endif // CAMERASTREAMER_HPP
