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
	std::chrono::microseconds pollPeriod;
	std::mutex &syncAsyncMutex;

public:
	template <class Rep, class Period>
	Task(asio::io_context &aContext, std::chrono::duration<Rep, Period> aPollPeriod, std::mutex &aSyncAsyncMutex):
		ioContext{aContext},
		pollPeriod{std::chrono::duration_cast<decltype(pollPeriod)>(aPollPeriod)},
		syncAsyncMutex{aSyncAsyncMutex}
	{
	}

	void iter();
	void run();
};

}  // namespace Socket

#endif  // SOCKET_PRIV_INCLUDE_TASK_HPP
