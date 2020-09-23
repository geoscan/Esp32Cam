#include <limits>
#include "UdpEndpoint.hpp"

UdpEndpoint::UdpEndpoint(asio::io_context &context, uint16_t port, size_t nMaxClients, size_t timeoutNoInputSec) :
	kTimeout(static_cast<Time>(timeoutNoInputSec) * 1000000),
	kMaxClients(nMaxClients > 0),
	socket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
{
}

size_t UdpEndpoint::read(asio::mutable_buffer buf)
{
	CliEndpoint sender;
	size_t      nrecv = socket.receive_from(buf, sender);

	if (!tryAccept(sender)) {
		nrecv = 0;
	}

	return nrecv;
}

size_t UdpEndpoint::write(asio::const_buffer buf)
{
	lock();

	auto   it    = clients.begin();
	size_t nsent = 0;

	for (; it != clients.end(); ++it) {
		if (expired(it->second)) {
			it = clients.erase(it);
		} else {
			nsent = socket.send_to(buf, it->first);
		}
	}

	unlock();

	return nsent;
}

bool UdpEndpoint::expired(Time time) const
{
	bool       res;
	const Time now    = esp_timer_get_time();
	const Time passed = (now < time) ? /*then*/ std::numeric_limits<Time>::max() - time +
		now - std::numeric_limits<Time>::min() : /*else*/ now - time;

	if (passed < kTimeout) {
		res = true;
	} else {
		res = false;
	}

	return res;
}

bool UdpEndpoint::tryAccept(CliEndpoint endpoint)
{
	bool res;

	if (clients.size() < kMaxClients) {
		res = true;
		auto itFind = clients.find(endpoint);
		if (itFind == clients.cend()) {
			lock();
			clients.emplace(endpoint, esp_timer_get_time());
			unlock();
		}
	} else {
		res = false;
	}

	return res;
}

void UdpEndpoint::lock()
{
	mutex.lock();
}

void UdpEndpoint::unlock()
{
	mutex.unlock();
}