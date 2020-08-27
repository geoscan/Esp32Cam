#ifndef RTSP_COMPONENTS_RTPPACKETSOURCECAMERA_HPP
#define RTSP_COMPONENTS_RTPPACKETSOURCECAMERA_HPP

#include "RtpPacketSource.hpp"
#include "Ov2640.hpp"

class RtpPsCamera : public RtpPacketSource {
public:
	Packets packets() override;
private:
};

#endif // RTSP_COMPONENTS_RTPPACKETSOURCECAMERA_HPP
