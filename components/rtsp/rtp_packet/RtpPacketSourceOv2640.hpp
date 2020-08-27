#ifndef COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEOV2640_HPP
#define COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEOV2640_HPP

#include "RtpPacketSource.hpp"
#include "Ov2640.hpp"
#include "esp_timer.h"

// DELETEME: esp_timer_get_time() -> time since boot up in us (10e-6)

class RtpPacketOv2640;


// -------------------- RtpPacketSourceOv2640 ------------------- //


/// Transmits mjpeg frames acquired via OV2640
class RtpPacketSourceOv2640 final : public RtpPacketSource {
public:
	RtpPacketSourceOv2640(unsigned fps = RtpPacketSourceOv2640::kDefaultFps);
	Packets packets() override;
private:
	Packets preparePackets(Ov2640::Image, unsigned curMs);

	static constexpr unsigned kDefaultFps = 10;
	const unsigned kFps;
	const unsigned kMsPerFrame = 1000 / (kFps + 1);
};


// ---------------------- RtpPacketOv2640 ----------------------- //


#endif // COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEOV2640_HPP
