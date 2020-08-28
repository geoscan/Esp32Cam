#ifndef COMPONENTS_RTSP_RTP_PACKET_RTPPACKETCHUNK_HPP
#define COMPONENTS_RTSP_RTP_PACKET_RTPPACKETCHUNK_HPP

#include "RtpPacketSource.hpp"

class RtpPacketChunk : public RtpPacket {
public:
	asio::const_buffer data() const override;
	RtpPacketChunk(asio::const_buffer bufToCopy);

	RtpPacketChunk(const RtpPacketChunk &) = delete;
	RtpPacketChunk(RtpPacketChunk &&) = delete;
	RtpPacketChunk &operator=(const RtpPacketChunk &) = delete;
	RtpPacketChunk &operator=(RtpPacketChunk &&) = delete;
private:
	const size_t len;
	std::unique_ptr<uint8_t[]> buf;
};

#endif // COMPONENTS_RTSP_RTP_PACKET_RTPPACKETCHUNK_HPP
