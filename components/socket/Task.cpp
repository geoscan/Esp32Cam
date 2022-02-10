//
// Task.cpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Task.hpp"

using namespace Sock;

void Task::iter()
{
	if (ioContext.stopped()) {
		ioContext.restart();
	}
	{
		std::lock_guard<std::mutex> lock{syncAsyncMutex};

		if (!ioContext.poll()) {
			std::this_thread::sleep_for(pollPeriod);
		}
	}
}

void Task::run() {
	while (true) {
		iter();
	}
}
