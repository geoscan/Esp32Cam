//
// UdpEndpoint.cpp
//
// Created on:  Sep 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <limits>
#include "utility/LockGuard.hpp"
#include "UdpEndpoint.hpp"



// ------------ UdpEndpoint ------------ //

UdpEndpoint::Time UdpEndpoint::time()
{
	return esp_timer_get_time();
}

UdpEndpoint::UdpEndpoint(asio::io_context &context, uint16_t port, size_t nMaxClients, size_t timeoutNoInputSec) :
	kTimeout(static_cast<Time>(timeoutNoInputSec) * 1000000),
	kMaxClients(nMaxClients),
	semaphore(xSemaphoreCreateCounting(kMaxClients, kMaxClients)),
	socket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
{
}

size_t UdpEndpoint::read(asio::mutable_buffer buf)
{
	CliEndpoint sender;
	size_t      nrecv = socket.receive_from(buf, sender);

	if (nrecv > 0) {
		const bool active   = cliMap.contains(sender) ? !expired(cliMap.at(sender)) : false;
		const Time timenow = time();

		// !active XOR (active AND haveRoomForHim) -> accept its message
		if (!active && addClient(true)) {
			cliMap.set(sender, timenow);
		} else if (active) {
			cliMap.set(sender, timenow);
		} else {  // otherwise -> ignore its message
			nrecv = 0;
		}
	}

	return nrecv;
}

size_t UdpEndpoint::write(asio::const_buffer buf)
{
	auto cliStack = cliMap.stack();

	while (!cliStack.empty()) {
		auto client = cliStack.pop();
//		volatile const bool exp = expired(client.second);
		if (expired(client.second)) {
			addClient(false);
		} else {
			asio::error_code err;
			socket.send_to(buf, client.first, 0, err);
		}
	}

	return 1;  // TODO: change to 'void'
}

bool UdpEndpoint::expired(Time time) const
{
	bool       isExpired;
	const Time now    = esp_timer_get_time();
	const Time passed = (now < time) ? /*then*/ std::numeric_limits<Time>::max() - time +
		now - std::numeric_limits<Time>::min() : /*else*/ now - time;  // Considering overflow

	if (passed > kTimeout) {
		isExpired = true;
	} else {
		isExpired = false;
	}

	return isExpired;
}

bool UdpEndpoint::addClient(bool fAdd)
{
	bool success = false;

	if (fAdd) {
		success = (xSemaphoreTake(semaphore, (TickType_t)0) == pdTRUE);  // Return instantly, if we can't acquire one
	} else {
		success = true;
		xSemaphoreGive(semaphore);
	}

	return success;
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


UdpEndpoint::Time &UdpEndpoint::CliMap::at(UdpEndpoint::CliEndpoint client)
{
	auto guard = Utility::makeLockGuard(mutex);
	return cliMap.at(client).get().second;
}

bool UdpEndpoint::CliMap::contains(UdpEndpoint::CliEndpoint client)
{
	auto guard = Utility::makeLockGuard(mutex);
	return (cliMap.find(client) != cliMap.end());
}

void UdpEndpoint::CliMap::set(UdpEndpoint::CliEndpoint client, UdpEndpoint::Time time)
{
	auto       guard    = Utility::makeLockGuard(mutex);
	const bool contains = (cliMap.find(client) != cliMap.end());

	if (!contains) {
		cliStack.emplace_back(client, time);
//		cliMap[client] = cliStack.back();
		cliMap.emplace(client, cliStack.back());
	} else {
		cliMap.at(client).get().second = time;
	}
}

UdpEndpoint::CliStack UdpEndpoint::CliMap::stack()
{
	auto guard = Utility::makeLockGuard(mutex);
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