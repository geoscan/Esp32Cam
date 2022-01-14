//
// Task.cpp
//
// Created on: Jan 14, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Task.hpp"

using namespace Socket;

void Task::operator()() {
	while (true) {
		if (ioContext.stopped()) {
			ioContext.restart();
		}
		if (!ioContext.poll()) {
			std::this_thread::sleep_for(pollPeriod);
		}
	}
}
