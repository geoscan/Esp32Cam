//
// RtpStreamer.hpp
//
// Created on:  Aug 25, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_RTSP_RTPSERVER_HPP
#define COMPONENTS_RTSP_RTPSERVER_HPP

#include "asio.hpp"
#include <map>
#include <list>
#include <utility>
#include <memory>
#include <set>
#include "Types.hpp"
#include "RtpPacketSource.hpp"

// RtpServer, sends RTP packages over UDP
class RtpServer final {
public:
	RtpServer(asio::io_context &, unsigned port);
	bool addStreamee(Rtsp::SessionId, std::unique_ptr<RtpPacketSource>, asio::ip::udp::endpoint recipient);
	/// Attach streamee to an existing 'RtpPacketSource'
	bool attachStreamee(unsigned packetSourceId, asio::ip::udp::endpoint recipient);
	bool removeStreamee(Rtsp::SessionId);
	bool setStreameeState(Rtsp::SessionId, bool isReceiving);
private:
	void stream();
	void syncQueue();

	asio::detail::mutex queueMutex;
};

#endif // COMPONENTS_RTSP_RTPSERVER_HPP
