#include "UartEndpoint.hpp"

size_t UartEndpoint::read(asio::mutable_buffer buf)
{
	return read(buf.data(), buf.size());
}

void UartEndpoint::write(asio::const_buffer buf)
{
	write(buf.data(), buf.size());
}