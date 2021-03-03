//
// CameraStream.hpp
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_OV2640_CAMERASTREAM_HPP
#define COMPONENTS_OV2640_CAMERASTREAM_HPP

#include <set>
#include <map>
#include <mutex>
#include <asio.hpp>

#include "utility/Subscription.hpp"

class CameraStream final : public Utility::Subscription::Sender {
public:
	void operator()();

	using Fps = short;

	// Use fps = -1 to disable artificial FPS limit
	static constexpr Fps kNoFpsLimit = -1;
	CameraStream(Fps fps = kNoFpsLimit);
	void addSubscriber(Utility::Subscription::Subscriber &s) override;
	void removeSubscriber(Utility::Subscription::Subscriber &s) override;

private:
	std::mutex mutex;
	Fps        fps;
};

#endif // COMPONENTS_OV2640_CAMERASTREAM_HPP
