#include <sstream>
#include <algorithm>
#include "rtsp.h"
#include "Media.hpp"
#include "RtpPacketSourceOv2640.hpp"
#include "ResponseComposer.hpp"

using namespace Rtsp;
using namespace std;

const Media::IsTypeCb Media::isType[] = {Media::isOv2640};
const Media::CreateCb Media::create[] = {Media::createOv2640};
const Media::GetSdpCb Media::sdp[]    = {Media::getSdpOv2640};

const size_t Media::kNtypes = sizeof(Media::isType);

bool Media::Stream::valid() const
{
	return (static_cast<bool>(source) || sourceId != RtpPacketSource::NoId);
}


std::vector<Media::Stream> Media::createStreams(const Request &req)
{
	size_t type;

	if (getType(req, type)) {
		return create[type](req);
	}

	return {};
}

std::string Media::getSdp(const Request &req)
{

	size_t type;

	if (getType(req, type)) {
		return sdp[type](req);
	}

	return {""};
}

bool Media::canCreateStreams(const Request &req)
{
	size_t type;
	return getType(req, type);
}

bool Media::getType(const Request &req, size_t &type)
{
	for (type = 0; type < kNtypes; ++type) {
		if (isType[type](req)) {
			return true;
		}
	}
	return false;
}


// ---------------------- Resource Ov2640 ----------------------- //


bool Media::isOv2640(const Request &req)
{
	bool res = true;

	res &= (req.hostResource.isVal() && req.hostResource.val().find("mjpeg") != std::string::npos);
	res &= (req.udp.isVal() && req.udp == true);

	return res;
}


struct IdInvalidator : RtpPacketSourceOv2640 {
	using RtpPacketSourceOv2640::RtpPacketSourceOv2640;

	static unsigned id;
	static asio::detail::mutex idMutex;
	~IdInvalidator()
	{
		Rtsp::LockGuard lockGuard(idMutex);
		id = RtpPacketSourceOv2640::NoId;
	}
};
asio::detail::mutex IdInvalidator::idMutex;
unsigned IdInvalidator::id = RtpPacketSource::NoId;


std::vector<Media::Stream> Media::createOv2640(const Request &req)
{
	if (!req.clientPort.isVal() || !req.clientAddress.isVal()) {
		return {}; // Not enough info
	}

	Rtsp::LockGuard lockGuard(IdInvalidator::idMutex);
	Stream stream;

	if (IdInvalidator::id == RtpPacketSource::NoId) {
		stream.source = std::unique_ptr<RtpPacketSource>(new IdInvalidator(false));
	} else {
		stream.sourceId = IdInvalidator::id;
	}
	stream.sink = asio::ip::udp::endpoint(req.clientAddress.val(), req.clientPort.val());

	vector<Stream> ret(1);
	ret.emplace_back(move(stream));
	return ret;
}

std::string Media::getSdpOv2640(const Request &req)
{
	using Rc = ResponseComposer;
	std::stringstream ss;

	return Rc::compose("v=0", Rc::kCrlf,
		"o=- ", esp_timer_get_time(), " 1 IN IP4 ", req.hostaddr.val(), ':', req.hostport.val(), Rc::kCrlf,
		"s=", Rc::kCrlf,
		"t=0 0", Rc::kCrlf,
		"m=video ", kRtpPort, " RTP/AVP/UDP", Rc::kCrlf,
		"c=IN IP 0.0.0.0", Rc::kCrlf);
}