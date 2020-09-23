#include "UartEndpoint.hpp"

size_t UartEndpoint::read(asio::mutable_buffer buf)
{
	return UartDevice::read(buf.data(), buf.size());
}

size_t UartEndpoint::write(asio::const_buffer buf)
{
	return UartDevice::write(buf.data(), buf.size());
}