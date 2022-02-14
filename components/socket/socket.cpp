//
// socket.cpp
//
// Created on: Feb 10, 2022
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include "socket/socket.hpp"
#include "socket/Api.hpp"
#include "Task.hpp"
#include <thread>
#include <chrono>

using namespace Sock;

static void apiInit(Sock::Api &aApi)
{
	constexpr auto kUdpOpen = {8001 /* AP protocol (MAVLink) */};

	asio::error_code err;

	for (auto port : kUdpOpen) {
		aApi.openUdp(port, err);
	}
}

void init(asio::io_context &aIoContext)
{
	static constexpr const std::chrono::milliseconds taskPollPeriod{200};
	static std::mutex syncAsyncMutex;
	static Task task{aIoContext, taskPollPeriod, syncAsyncMutex};
	static Sock::Api api{aIoContext, syncAsyncMutex};

	apiInit(api);

	static std::thread thread{&Task::run, &task};
	(void)api;
	(void)thread;
}

void init()
{
	static asio::io_context ioContext;
	Sock::init(ioContext);
}
