#include "RtpPacketSourceFactory.hpp"
#include "RtpPacketSourceOv2640.hpp"

std::unique_ptr<RtpPacketSource> RtpPacketSourceFactory::create(const Rtsp::Request &req, unsigned &idCreated)
{
	size_t type = detectType(req);
	std::unique_ptr<RtpPacketSource> ptr;
	idCreated = 0;

	if (type == static_cast<size_t>(Undefined)) {
		return ptr;
	}

	construct[type](req, ptr, idCreated);

	return ptr;
}

void RtpPacketSourceFactory::constructOv2640(const Rtsp::Request &req,
	std::unique_ptr<RtpPacketSource> &ptr, unsigned &id)
{
	static RtpPacketSourceOv2640 *instance = nullptr;
	static unsigned sId = 0;

	if (!instance) {
		ptr = move(std::unique_ptr<RtpPacketSource>(new RtpPacketSourceOv2640(!req.udp.val())));
		sId = ptr->id();
		id  = 0;
	} else {
		id = sId;
	}
}

bool RtpPacketSourceFactory::isTypeOv2640(const Rtsp::Request &req)
{
	bool res = true;

	res &= (req.format.isVal() && req.format.val() == Rtsp::Format::Mjpeg);
	res &= (req.udp.isVal() && req.udp == true);

	return res;
}

//std::unique_ptr<RtpPacketSource> RtpPacketSourceFactory::create(const Rtsp::Request &req, unsigned &idCreated)
//{
//	std::unique_ptr<RtpPacketSource> source;

//	construct[detectType(req)](req, source, idCreated);

//	return source;
//}

size_t RtpPacketSourceFactory::detectType(const Rtsp::Request &req)
{
	size_t type = SourceType::Ov2640;
	for (; type < SourceType::Undefined; ++type) {
		if (isType[type](req)) {
			return type;
		}
	}
	return type;
}

