//
// CameraStream.hpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_OV2640_CAMERASTREAM_HPP
#define COMPONENTS_OV2640_CAMERASTREAM_HPP

#include "asio.hpp"
#include <set>

// Wrapper around UDP socket that
// sends JPEG frames to its sinks
class CameraStream final {
public:
	using Fps = short;
	/// @param fps <= 0 -- Instant frame sending
	///            >  0 -- Use provided value as max FPS
	CameraStream(asio::io_context &context, uint16_t sourcePort, Fps fps = -1);
	void run();
	void addSink(asio::ip::udp::endpoint);
	void removeSink(asio::ip::udp::endpoint);
	void removeSinks();
private:
	static uint32_t currentTimeMs();

	asio::ip::udp::socket             socket;
	std::set<asio::ip::udp::endpoint> sinks; // Client endpoints
	asio::detail::mutex               mutex;
	const Fps                         fps;
};

#endif // COMPONENTS_OV2640_CAMERASTREAM_HPP
