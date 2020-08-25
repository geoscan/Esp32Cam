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
#include <vector>

class RtpPacket {
public:
	virtual asio::const_buffer data() const = 0;
	virtual ~RtpPacket()
	{
	}
};


// WARN: The class is not thread safe,
// custom thread-safety facilities are required
class RtpPacketSource {
public:

	RtpPacketSource(const unsigned /*id*/);

	virtual std::vector<std::unique_ptr<RtpPacket>> packets() = 0;
	virtual ~RtpPacketSource()
	{
	}
	unsigned id() const;
private:
	static unsigned idBound;
	const unsigned nId;
};


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

#endif // COMPONENTS_RTSP_RTPPACKETESOURCE_HPP
