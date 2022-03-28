//
// CriticalLock.cpp
//
// Created on: Mar 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include "utility/CriticalLock.hpp"
#include <esp_log.h>


namespace Utility {

CriticalLock::CriticalLock(Critical &aLock) : lock{aLock}
{
	vTaskEnterCritical(&lock.lock);
}

CriticalLock::~CriticalLock()
{
	vTaskExitCritical(&lock.lock);
}

Critical::Critical() : lock(portMUX_INITIALIZER_UNLOCKED)
{
}

}  // namespace Utility

