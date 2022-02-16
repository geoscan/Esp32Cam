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
	bool fWait = false;
	{
		std::lock_guard<std::mutex> lock{syncAsyncMutex};

		if (!ioContext.poll()) {
			fWait = true;
		}
	}
	if (fWait) {
		Utility::waitMs(std::chrono::duration_cast<std::chrono::milliseconds>(pollPeriod).count());
	}
}

void Task::run() {
	while (true) {
		iter();
	}
}
