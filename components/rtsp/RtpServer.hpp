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
#include <utility>
#include <memory>
#include <set>
#include <list>
#include "Types.hpp"
#include "RtpPacketSource.hpp"

class RtpServer final {
public:
	RtpServer(asio::io_context &, unsigned port);
	bool addSession(Rtsp::SessionId, std::unique_ptr<RtpPacketSource>, asio::ip::udp::endpoint recipient);
	/// Attach streamee to an existing 'RtpPacketSource'
	bool addSession(Rtsp::SessionId, unsigned packetSourceId, asio::ip::udp::endpoint recipient);
	bool removeSession(Rtsp::SessionId);
	bool setSessionStreaming(Rtsp::SessionId, bool enableStreaming);

private:
	void stream();
	void sync();

	// Streams storage and relevant operations

	using Sinks    = std::set<asio::ip::udp::endpoint>;
	using Sessions = std::map<Rtsp::SessionId, std::pair<unsigned /*source id*/, asio::ip::udp::endpoint>,
		RtpPacketSource::Less>; // Registered sessions
	using Streams  = std::map<std::unique_ptr<RtpPacketSource>, Sinks>; // Ongoing streams

	Streams streams;
	Sessions sessions;

	bool registerSession(Rtsp::SessionId, unsigned packetSourceId, asio::ip::udp::endpoint recipient);
//	bool removeSink(unsigned sourceId, asio::ip::udp::endpoint);

	struct Streamee : public Rtsp::Identifiable<Streamee>{
		asio::ip::udp::endpoint address;
		bool isReceiving;
	};

	// -------------------- Info. about streams --------------------  //

	std::set<std::unique_ptr<RtpPacketSource>> streamSources;

	// Indexes
	std::map<unsigned /*id of RtpPacketSource*/, std::vector<Streamee>> packetSourceIdToStreamee;
	std::map<Rtsp::SessionId, unsigned /*id of RtpPacketSource*/> sessionIdToPacketSourceId;

	asio::detail::mutex queueMutex;
};

#endif // COMPONENTS_RTSP_RTPSERVER_HPP
