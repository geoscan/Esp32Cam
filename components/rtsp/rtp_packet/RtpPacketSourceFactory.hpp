#ifndef COMPONENTS_RTSP_RTPPACKETSOURCEFACTORY_HPP
#define COMPONENTS_RTSP_RTPPACKETSOURCEFACTORY_HPP

#include "RtpPacketSource.hpp"

class RtpPacketSourceFactory final {
public:
	/// @brief Creates a new instance of 'RtpPacketSource'.
	///        The exact type depends on 'Rtsp::Request &'
	static std::unique_ptr<RtpPacketSource> create(const Rtsp::Request &);

	/// @brief  Depending on 'Rtsp::Request',
	///	        returns the 'idCreated' of the last created
	///         instance  of 'RtpPacketSource', or
	///         OTHERWISE creates a new instance.
	/// @return <nullptr>, AND 'idCreated=0', on error.
	///         <ptr-to-created-instance> or 'idCreated' OTHERWISE
	static std::unique_ptr<RtpPacketSource> create(const Rtsp::Request &, unsigned &idCreated);
};


#endif // COMPONENTS_RTSP_RTPPACKETSOURCEFACTORY_HPP
