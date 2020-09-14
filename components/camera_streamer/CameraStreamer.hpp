#ifndef CAMERASTREAMER_HPP
#define CAMERASTREAMER_HPP

#include "asio.hpp"

class CameraStreamer final {
public:
	static constexpr const uint16_t kDefaultFps = 15;
	CameraStreamer(asio::io_context &context, uint16_t sourcePort /*this port*/,
		uint16_t sinkPort, uint16_t fps = CameraStreamer::kDefaultFps);
	void run();
private:
	static uint32_t currentTimeMs();

	asio::ip::udp::socket   socket;
	asio::ip::udp::endpoint sink;
	const uint16_t          fps;
};

#endif // CAMERASTREAMER_HPP
