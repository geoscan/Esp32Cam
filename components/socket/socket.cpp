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

void init(asio::io_context &aIoContext)
{
	static constexpr const std::chrono::milliseconds taskPollPeriod{200};
	static std::mutex syncAsyncMutex;
	static Task task{aIoContext, taskPollPeriod, syncAsyncMutex};
	static Sock::Api api{aIoContext, syncAsyncMutex};
	static std::thread thread{&Task::run, &task};
	(void)api;
	(void)thread;
}

void init()
{
	static asio::io_context ioContext;
	Sock::init(ioContext);
}
