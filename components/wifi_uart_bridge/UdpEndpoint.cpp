//
// UdpEndpoint.cpp
//
// Created on:  Sep 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <limits>
#include "utility/thr/LockGuard.hpp"
#include "utility/al/Time.hpp"
#include "UdpEndpoint.hpp"


// ------------ UdpEndpoint ------------ //

UdpEndpoint::UdpEndpoint(asio::io_context &context, uint16_t port, size_t nMaxClients, size_t timeoutNoInputSec) :
	kTimeout(static_cast<Ut::Time>(timeoutNoInputSec) * 1000000),
	kMaxClients(nMaxClients),
	socket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
{
}

size_t UdpEndpoint::read(asio::mutable_buffer buf)
{
	CliEndpoint sender;
	size_t      nrecv = socket.receive_from(buf, sender);

	cliMap.set(sender, Ut::bootTimeUs());

	return nrecv;
}

size_t UdpEndpoint::write(asio::const_buffer buf)
{
	auto cliStack = cliMap.stack();

	while (!cliStack.empty()) {
		auto client = cliStack.pop();
		if (!Ut::Al::expired(client.second, kTimeout)) {
			asio::error_code err;
			socket.send_to(buf, client.first, 0, err);
		}
	}

	return 1;  // TODO: change to 'void'
}



// ------------ CliMap and CliStack ------------ //
//
// Concept:
// The system has to maintain info about client's timeouts.
// Using associative arrays <address,timeout> is the most effective and
// convenient way to do so. But iteration through an associative array leads
// to resource starvation, as mutex is being locked/unlocked too fast, and
// there's no other way to do so as the array is usually implemented as
// R/B trees, which implies lack of thread safety.
//
// So it's been decided to create a redundant container with a
// (1) growing-ONLY stack (CliMap::cliStack) which may be safely appended to
// and iterated though in separate threads
// (2) registry of connected clients, to prevent creation (CliMap::cliMap)
// of already existing clients.
//
// CliMap is an encapsulation of this approach.
// CliStack is a pseudo stack which in fact is a wrapper
// over CliMap::cliMap::iterator.

// ------------ CliMap ------------ //


Ut::Time &UdpEndpoint::CliMap::at(UdpEndpoint::CliEndpoint client)
{
	auto guard = Ut::makeLockGuard(mutex);
	return cliMap.at(client).get().second;
}

bool UdpEndpoint::CliMap::contains(UdpEndpoint::CliEndpoint client)
{
	auto guard = Ut::makeLockGuard(mutex);
	return (cliMap.find(client) != cliMap.end());
}

void UdpEndpoint::CliMap::set(UdpEndpoint::CliEndpoint client, Ut::Time time)
{
	auto       guard    = Ut::makeLockGuard(mutex);
	const bool contains = (cliMap.find(client) != cliMap.end());

	if (!contains) {
		cliStack.emplace_back(client, time);
		cliMap.emplace(client, cliStack.back());
	} else {
		cliMap.at(client).get().second = time;
	}
}

UdpEndpoint::CliStack UdpEndpoint::CliMap::stack()
{
	auto guard = Ut::makeLockGuard(mutex);
	return CliStack(cliStack.begin(), cliStack.size());
}


// ------------ CliStack ------------ //


UdpEndpoint::CliStack::CliStack(std::list<UdpEndpoint::CliInfo>::iterator it, size_t nclients) :
	iter(it),
	size(nclients)
{
}

bool UdpEndpoint::CliStack::empty() const
{
	return (size == 0);
}

UdpEndpoint::CliInfo &UdpEndpoint::CliStack::pop()
{
	size--;
	return *iter++;
}
