#ifndef COMPONENTS_RTSP_RTSP_MEDIA_HPP
#define COMPONENTS_RTSP_RTSP_MEDIA_HPP

#include <utility>
#include <vector>
#include <array>

#include "Types.hpp"
#include "RtpPacketSource.hpp"
#include "ResponseComposer.hpp"
#include "asio.hpp"

namespace Rtsp {

/// Uses Rtsp::Request to create 'RtpPacketSource's
/// and their meta-info, such as SDP headers.
class Media {
public:
	struct Stream {
		Rtsp::SessionId sessionId;
		std::unique_ptr<RtpPacketSource> source;
		unsigned sourceId;
		asio::ip::udp::endpoint sink;
		bool valid();
	};

	// https://tools.ietf.org/html/rfc2326#page-31 -- RTSP: rescribe resource
	// https://tools.ietf.org/html/rfc4566 -- Session Description Protocol
	/// @brief Get media description accord. to rfc4566
	std::string getSdp(const Rtsp::Request &);

	std::vector<Stream> createStreams(const Request &);
	bool canCreateStreams(const Request &); ///< Is there any resource satisfying client's request
private:
	bool getType(const Request &, size_t &type);

	// callbacks

	// Ov2640
	static bool isOv2640(const Request &);
	static std::vector<Stream> createOv2640(const Request &);
	static std::string getSdpOv2640(const Request &);

	using IsTypeCb = bool(*)(const Request &);
	using CreateCb = std::vector<Stream>(*)(const Request &);
	using GetSdpCb = std::string(*)(const Request &);

	static const IsTypeCb isType[];
	static const CreateCb create[];
	static const GetSdpCb sdp[];

	static const size_t kNtypes;
};

} // Rtsp

#endif // COMPONENTS_RTSP_RTSP_MEDIA_HPP
