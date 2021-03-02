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
#include <map>
#include <mutex>

// Wrapper around UDP socket that
// sends JPEG frames to its sinks
class CameraStream final {
public:
	using Fps = short;
	/// @param fps <= 0 -- Instant frame sending
	///            >  0 -- Use provided value as max FPS
	CameraStream(asio::io_context &context, uint16_t sourcePort, Fps fps = -1);
	void run();
	void removeSink(const asio::ip::address &);
	void addSink(const asio::ip::address &addr, short unsigned port);
	void removeSinks();
private:
	using Sinks = std::map<asio::ip::address, short unsigned>;
	asio::ip::udp::socket socket;
	Sinks                 sinks;
	std::mutex            mutex;
	const Fps             fps;
};

#endif // COMPONENTS_OV2640_CAMERASTREAM_HPP
