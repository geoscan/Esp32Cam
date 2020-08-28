#ifndef COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEOV2640_HPP
#define COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEOV2640_HPP

#include "RtpPacketSource.hpp"
#include "Ov2640.hpp"


/// Transmits mjpeg frames acquired using OV2640
class RtpPacketSourceOv2640 final : public RtpPacketSource {
public:
	RtpPacketSourceOv2640(bool packForTcp, unsigned fps = RtpPacketSourceOv2640::kDefaultFps);
	Packets packets() override;
private:
	using BufPtr    = const unsigned char *;
	using Timestamp = uint32_t;

	static constexpr unsigned kDefaultFps = 25;

	static Timestamp currentTimeMs();

	bool ready() const;
	void updateTimestamp();
	std::unique_ptr<RtpPacket> nextPacket(unsigned const char *jpeg, size_t jpegLen, unsigned &offset,
		BufPtr quantTbl0, BufPtr quantTbl1, uint16_t width, uint16_t height);


	// When JPEG is stored as a file it is wrapped in a container
	// This function fixes up the provided start ptr to point to the
	// actual JPEG stream data and returns the number of bytes skipped
	// returns true if the file seems to be valid jpeg
	// If quant tables can be found they will be stored in qtable0/1
	static bool decodeJpegFile(BufPtr *start, uint32_t *len, BufPtr *qtable0, BufPtr *qtable1);
	static bool findJpegHeader(BufPtr *start, uint32_t *len, uint8_t marker);

	// Given a jpeg ptr pointing to a pair of length bytes, advance the pointer to
	// the next 0xff marker byte
	static void nextJpegBlock(BufPtr *start);
	static void skipScanBytes(BufPtr *start);

	const unsigned kFps;
	const unsigned kMinMsPerFrame = 1000 / kFps;
	uint32_t       mSequenceNumber;
	Timestamp      mTimestamp;
	bool           mTcpTransport;
	Timestamp      mPrevMs;
};

#endif // COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEOV2640_HPP
