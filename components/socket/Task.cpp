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
	ioContext.poll();
}

void Task::run() {
	while (true) {
		iter();
		vTaskDelay(1);
	}
}
