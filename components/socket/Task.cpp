//
// Task.cpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Task.hpp"
#include "utility/time.hpp"

using namespace Sock;

void Task::iter()
{
	if (ioContext.stopped()) {
		ioContext.restart();
	}
	{
		std::lock_guard<std::mutex> lock{syncAsyncMutex};
		asio::error_code err;

		if (!ioContext.poll_one(err)) {
			Utility::waitMs(std::chrono::duration_cast<std::chrono::milliseconds>(pollPeriod).count());
		}

	}
}

void Task::run() {
	while (true) {
		iter();
	}
}
