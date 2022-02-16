//
// Task.hpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SOCKET_PRIV_INCLUDE_TASK_HPP
#define SOCKET_PRIV_INCLUDE_TASK_HPP

#include <asio.hpp>
#include <chrono>
#include <thread>
#include <mutex>

namespace Sock {

class Task {
	asio::io_context &ioContext;
	std::mutex &syncAsyncMutex;

public:
	Task(asio::io_context &aContext, std::mutex &aSyncAsyncMutex):
		ioContext{aContext},
		syncAsyncMutex{aSyncAsyncMutex}
	{
	}

	void iter();
	void run();
};

}  // namespace Socket

#endif  // SOCKET_PRIV_INCLUDE_TASK_HPP
