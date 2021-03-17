//
// CameraStream.hpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_OV2640_CAMERASTREAM_HPP
#define COMPONENTS_OV2640_CAMERASTREAM_HPP


#include <asio.hpp>
#include <set>
#include <map>
#include <mutex>

#include "Ov2640.hpp"
#include "Messaging.hpp"

namespace CameraStreamer {

class CameraStream final {
public:
	void operator()();

	using Fps = short;

	// Use fps = -1 to disable artificial FPS limit
	static constexpr Fps kNoFpsLimit = -1;
	CameraStream(Fps fps = kNoFpsLimit);

private:
	CameraStreamer::Key::NewFrame key;
	Fps fps;
};

}  // namespace CameraStreamer

#endif // COMPONENTS_OV2640_CAMERASTREAM_HPP
