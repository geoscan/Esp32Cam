//
// RtpPackageSource.hpp
//
// Created on:  Aug 25, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTPPACKETESOURCE_HPP
#define COMPONENTS_RTSP_RTPPACKETESOURCE_HPP

#include "asio.hpp"
#include <memory>
#include <vector>
#include "Types.hpp"

class RtpPacket {
public:
	virtual asio::const_buffer data() const = 0;
	virtual ~RtpPacket()
	{
	}
};


// WARN: The class is not thread safe,
// custom thread-safety facilities are required
class RtpPacketSource : public Rtsp::Identifiable<RtpPacketSource> {
public:
	virtual std::vector<std::unique_ptr<RtpPacket>> packets() = 0;
	virtual ~RtpPacketSource()
	{
	}
};

#endif // COMPONENTS_RTSP_RTPPACKETESOURCE_HPP
