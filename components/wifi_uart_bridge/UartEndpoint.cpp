#include "UartEndpoint.hpp"

size_t UartEndpoint::read(asio::mutable_buffer buf)
{
	return read(buf.data(), buf.size());
}

size_t UartEndpoint::write(asio::const_buffer buf)
{
	return write(buf.data(), buf.size());
}