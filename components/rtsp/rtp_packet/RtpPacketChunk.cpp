#include "RtpPacketChunk.hpp"

RtpPacketChunk::RtpPacketChunk(asio::const_buffer cb) :
	len(cb.size()),
	buf(new uint8_t[len])
{
	asio::mutable_buffer mb(buf.get(), len);
	asio::buffer_copy(mb, cb);
}

asio::const_buffer RtpPacketChunk::data() const
{
	return {buf.get(), len};
}