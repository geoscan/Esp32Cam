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
	kMaxClients(nMaxClients > 0),
	semaphore(xSemaphoreCreateCounting(kMaxClients, 0)),
	socket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
{
}

size_t UdpEndpoint::read(asio::mutable_buffer buf)
{
	CliEndpoint sender;
	size_t      nrecv    = socket.receive_from(buf, sender);
	const bool  contains = cliMap.contains(sender);
	Time        timeNow  = time();

	if (!contains && addClient(true)) {  // New client, eligible to be added to active group
		cliMap.add(sender, timeNow);
	} else if (contains && !expired(cliMap.timestamp(sender))) {  // Existing active client
		cliMap.setTimestamp(sender, timeNow);
	} else {
		nrecv = 0;
	}

	return nrecv;
}

size_t UdpEndpoint::write(asio::const_buffer buf)
{
	auto cliStack = cliMap.stack();

	while (!cliStack.empty()) {
		auto client = cliStack.pop();
		if (!expired(client.second)) {
			socket.send_to(buf, client.first);
		}
	}

	return 1;  // TODO: change to 'void'
}

bool UdpEndpoint::expired(Time time) const
{
	bool       isExpired;
	const Time now    = esp_timer_get_time();
	const Time passed = (now < time) ? /*then*/ std::numeric_limits<Time>::max() - time +
		now - std::numeric_limits<Time>::min() : /*else*/ now - time;

	if (passed > kTimeout) {
		isExpired = true;
	} else {
		isExpired = false;
	}

	return isExpired;
}

bool UdpEndpoint::reachedCapacity()
{
	return (uxSemaphoreGetCount(semaphore) == kMaxClients);
}

bool UdpEndpoint::addClient(bool fAdd)
{
	bool success = false;

	if (fAdd) {
		success = xSemaphoreTake(semaphore, 0);  // Return instantly, if we can't acquire one
	} else {
		success = true;
		xSemaphoreGive(semaphore);
	}

	return success;
}


// ------------ CliMap ------------ //


UdpEndpoint::Time &UdpEndpoint::CliMap::timestamp(UdpEndpoint::CliEndpoint client)
{
//	auto    guard = Utility::makeLockGuard(mutex);
//	return cliMap[client].get().second;
	static Time t;
	return t;
}

void UdpEndpoint::CliMap::setTimestamp(UdpEndpoint::CliEndpoint client, UdpEndpoint::Time time)
{
	add(client, time);
}

bool UdpEndpoint::CliMap::contains(UdpEndpoint::CliEndpoint client)
{
	auto guard = Utility::makeLockGuard(mutex);
	return (cliMap.find(client) != cliMap.end());
}

void UdpEndpoint::CliMap::add(UdpEndpoint::CliEndpoint client, UdpEndpoint::Time time)
{
	auto       guard    = Utility::makeLockGuard(mutex);
	const bool contains = (cliMap.find(client) != cliMap.end());

	if (!contains) {
		cliStack.emplace_back(client, time);
//		cliMap[client] = cliStack.back();
		cliMap.emplace(client, cliStack.back());
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
	return *iter++;
}