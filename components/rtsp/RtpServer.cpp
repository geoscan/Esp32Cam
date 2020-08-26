#include <utility>
#include "RtpServer.hpp"

using namespace std;

RtpServer::RtpServer(asio::io_context &, unsigned /*port*/)
{
}

bool RtpServer::addSession(Rtsp::SessionId sid, std::unique_ptr<RtpPacketSource> source,
	asio::ip::udp::endpoint recipient)
{
	Rtsp::LockGuard lockGuard{queueMutex};

	auto sourceId = source->id();

	// Add a new packet source
	streams.emplace(make_pair(move(source), Sinks{}));

	// Do all the rest
	if (!registerSession(sid, sourceId, recipient)) {
		return false;
	}

	return true;
}

bool RtpServer::addSession(Rtsp::SessionId sid, unsigned sourceId, asio::ip::udp::endpoint recipient)
{
	Rtsp::LockGuard lockGuard{queueMutex};

	return registerSession(sid, sourceId, recipient);
}

bool RtpServer::removeSession(Rtsp::SessionId sid)
{
	Rtsp::LockGuard lockGuard{queueMutex};

	auto itSess = sessions.find(sid);

	if (itSess == sessions.end()) {
		return false; // Sessions does not exist
	}

	auto sourceId = itSess->second.first;
	auto sink = itSess->second.second;
	auto sourceKey(RtpPacketSource::key(sourceId));

	auto itStream = streams.find(sourceKey); // Source is guaranteed to be there
	auto itSink = itStream->second.find(sink);

	if (itSink != itStream->second.end()) { // Remove a sink from the queue
		itStream->second.erase(itSink);
	}

	sessions.erase(itSess);

	return true;
}

bool RtpServer::setSessionStreaming(Rtsp::SessionId sid, bool enableStreaming)
{
	Rtsp::LockGuard lockGuard{queueMutex};
	auto itSess = sessions.find(sid);

	if (itSess == sessions.end()) {
		return false; // Sessions does not exist
	}

	unsigned sourceId = itSess->second.first;
	asio::ip::udp::endpoint ep = itSess->second.second;
	auto key(RtpPacketSource::key(sourceId));
	auto itStream = streams.find(key); // Source is guaranteed to be there

	Sinks &sinks = itStream->second;

	if (enableStreaming) {
		sinks.insert(ep);
	} else {
		auto itSink = sinks.find(ep);
		if (itSink != sinks.end()) {
			sinks.erase(itSink);
		}
	}
	return true;
}


// -------------------------- Private --------------------------- //


bool RtpServer::registerSession(Rtsp::SessionId sid, unsigned sourceId, asio::ip::udp::endpoint recipient)
{
	if (sessions.find(sid) != sessions.end()) {
		return false; // Session already exists
	}

	auto key(RtpPacketSource::key(sourceId));
	if (streams.find(key) == streams.end()) {
		return false; // Could not find media source
	}

	sessions.emplace(make_pair(sid, make_pair(sourceId, recipient)));
	return true;
}