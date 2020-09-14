#include <freertos/FreeRTOS.h>
#include <esp_timer.h>

#include <memory>

#include "CameraStreamer.hpp"
#include "Ov2640.hpp"

using asio::ip::udp;
using namespace std;

uint32_t CameraStreamer::currentTimeMs()
{
	return static_cast<uint32_t>(esp_timer_get_time() * 1000);
}

CameraStreamer::CameraStreamer(asio::io_context &context, uint16_t sourcePort,
	uint16_t sinkPort, Fps f) :
	socket(context, udp::endpoint(udp::v4(), sourcePort)),
	sink(udp::v4(), sinkPort),
	fps(f)
{
}



void CameraStreamer::run()
{
	using Time = decltype(currentTimeMs());
	static const auto kWaitMs = 1000 / fps;

	auto img = Ov2640::instance().jpeg();
	Time timeLast;

	do {
		// Send JPEG
		if (fps) {
			timeLast = currentTimeMs();
		}
		socket.send_to(img->data(), sink);
		img = Ov2640::instance().jpeg();

		if (fps) {
			// Wait until we are eligible for sending the next frame
			auto timeNow = currentTimeMs();
			auto timedelta = (timeNow < timeLast /*overflow*/) ? kWaitMs / 2 : timeNow - timeLast;
			vTaskDelay((kWaitMs - timedelta) / portTICK_PERIOD_MS);
		}
	} while (img->valid());
}