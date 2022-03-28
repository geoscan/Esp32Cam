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
#include "utility/Threading.hpp"

namespace Sock {

static void apiInit(Sock::Api &aApi)
{
	constexpr auto kUdpOpen = {8001 /* AP protocol (MAVLink) */};
	asio::error_code err;

	for (auto port : kUdpOpen) {
		aApi.openUdp(port, err);
	}

	aApi.openTcp(8889, err);
}

void start(asio::io_context &aIoContext)
{
	static std::mutex syncAsyncMutex;
	static Task task{aIoContext};
	static Sock::Api api{aIoContext, syncAsyncMutex};

	apiInit(api);

	Utility::Threading::Config(true).core(0).stack(CONFIG_ESP32_PTHREAD_TASK_STACK_SIZE_DEFAULT + 2048);
	static std::thread thread{&Task::run, &task};
	(void)api;
	(void)thread;
}

void start()
{
	static asio::io_context ioContext;
	Sock::start(ioContext);
}

}  // namespace Sock
