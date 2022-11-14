//
// OsApi.cpp
//
// Created on: Oct 04, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "OsApi.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace Trk {

OsApi::OsApi()
{
	setInstance(*this);
}

void OsApi::taskYieldMinDelay()
{
	vTaskDelay(1);
}

}  // namespace Trk
