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
#include <queue>
#include "Types.hpp"

class RtpPacket {
public:
	virtual asio::const_buffer data() const = 0;
	virtual ~RtpPacket()
	{
	}
};

using Packets = std::queue<std::shared_ptr<RtpPacket>>;

class RtpPacketSource : public Rtsp::Identifiable<RtpPacketSource> {
public:
	virtual std::queue<std::shared_ptr<RtpPacket>> packets() = 0;
	virtual ~RtpPacketSource()
	{
	}
};

#endif // COMPONENTS_RTSP_RTPPACKETESOURCE_HPP
