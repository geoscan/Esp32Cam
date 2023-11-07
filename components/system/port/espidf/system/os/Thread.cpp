//
// Thread.cpp
//
// Created on: Oct 24, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "system/os/Thread.hpp"

namespace Sys {

void Thread::delayMs(std::size_t aMilliseconds)
{
	vTaskDelay(pdMS_TO_TICKS(aMilliseconds));
}

}  // Sys
