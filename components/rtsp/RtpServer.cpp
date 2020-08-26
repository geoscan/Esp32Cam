#include <tuple>
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
	unsigned        sourceId;
	Endpoint        ep;

	tie(sourceId, ep) = session(sid);

	if (sourceId == 0) {
		return false;
	}
	detachSink(sourceId, ep);

	sessions.erase(sid);

	return true;
}

bool RtpServer::setSessionStreaming(Rtsp::SessionId sid, bool enableStreaming)
{
	Rtsp::LockGuard lockGuard{queueMutex};
	unsigned        sourceId;
	Endpoint        ep;

	tie(sourceId, ep) = session(sid);

	if (sourceId == 0) {
		return false;
	}

	if (enableStreaming) {
		attachSink(sourceId, ep);
	} else {
		detachSink(sourceId, ep);
	}

	return true;
}


// -------------------------- Private --------------------------- //


bool RtpServer::registerSession(Rtsp::SessionId sid, unsigned sourceId, Endpoint recipient)
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

pair<unsigned, RtpServer::Endpoint> RtpServer::session(Rtsp::SessionId sid)
{
	auto itSess = sessions.find(sid);

	if (itSess == sessions.end()) {
		return {RtpPacketSource::NoId, {}};
	}

	return itSess->second;
}

void RtpServer::detachSink(unsigned sourceId, Endpoint ep)
{
	auto key(RtpPacketSource::key(sourceId));
	auto itStream = streams.find(key);

	if (itStream == streams.end()) {
		return;
	}

	itStream->second.erase(ep);
}

void RtpServer::attachSink(unsigned sourceId, Endpoint ep)
{
	auto key(RtpPacketSource::key(sourceId));
	auto itStream = streams.find(key);

	if (itStream == streams.end()) {
		return;
	}

	itStream->second.emplace(ep);
}