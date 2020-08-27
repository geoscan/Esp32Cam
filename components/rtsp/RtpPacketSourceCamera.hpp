#ifndef RTSP_COMPONENTS_RTPPACKETSOURCECAMERA_HPP
#define RTSP_COMPONENTS_RTPPACKETSOURCECAMERA_HPP

#include "RtpPacketSource.hpp"

class RtpPsCamera : public RtpPacketSource {
public:
	std::shared_ptr<RtpPacket> packet() override;
};

#endif // RTSP_COMPONENTS_RTPPACKETSOURCECAMERA_HPP
