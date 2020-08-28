#ifndef COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEFACTORY_HPP
#define COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEFACTORY_HPP

#include "RtpPacketSource.hpp"
#include <array>
#include <stack>

class RtpPacketSourceFactory final {
public:
	/// @brief  Detects an appropriate type and either creates an
	///         instance of it or returns the id of a previously
	///         created instance
	///
	/// @return <nullptr>, AND 'idCreated=0', on error.
	///         <ptr-to-created-instance> or 'idCreated' OTHERWISE
	std::unique_ptr<RtpPacketSource> create(const Rtsp::Request &, unsigned &idCreated);
private:
	enum SourceType : size_t {
		Ov2640 = 0,

		Undefined,
	};
	using IsType    = bool(*)(const Rtsp::Request &);
	using Construct = void(*)(const Rtsp::Request &, std::unique_ptr<RtpPacketSource> &ptr, unsigned &id);

	template <typename T>
	using TypeArray = std::array<T, (size_t)Undefined>;

	static bool isTypeOv2640(const Rtsp::Request &);
	static void constructOv2640(const Rtsp::Request &, std::unique_ptr<RtpPacketSource> &ptr, unsigned &id);

	// Arrays of callbacks
	static constexpr TypeArray<IsType>    isType   {{isTypeOv2640}};
	static constexpr TypeArray<Construct> construct{{constructOv2640}};

	size_t detectType(const Rtsp::Request &);
//	std::unique_ptr<RtpPacketSource> create(SourceType, const Rtsp::Request &);
};

#endif // COMPONENTS_RTSP_RTP_PACKET_RTPPACKETSOURCEFACTORY_HPP
