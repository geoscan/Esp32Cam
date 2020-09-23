#include <limits>
#include "utility/LockGuard.hpp"
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
	volatile size_t      nrecv = socket.receive_from(buf, sender);

	if (!tryAccept(sender)) {
		nrecv = 0;
	}

	return nrecv;
}

size_t UdpEndpoint::write(asio::const_buffer buf)
{
	size_t nsent = 0;
	decltype(clients) cpClients;

	{  // Copy clients for sake of thread safety
		auto guard = Utility::makeLockGuard(mutex);
		cpClients = clients;
	}

	for (auto it = cpClients.begin(); it != cpClients.end(); ++it) {
		if (expired(it->second)) {
			it = cpClients.erase(it);
		} else {
			nsent = socket.send_to(buf, it->first);
		}
	}

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
		bool exists = false;

		{
			auto guard = Utility::makeLockGuard(mutex);
			exists = (clients.find(endpoint) == clients.end());
		}

		if (!exists) {
			auto guard = Utility::makeLockGuard(mutex);
			clients.emplace(endpoint, esp_timer_get_time());
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