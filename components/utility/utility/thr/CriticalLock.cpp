//
// CriticalLock.cpp
//
// Created on: Mar 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <freertos/FreeRTOS.h>
#include "utility/thr/CriticalLock.hpp"
#include <esp_log.h>


namespace Ut {

CriticalLock::CriticalLock(Critical &aLock) : lock{aLock}
{
	vPortEnterCritical(&lock.lock);
}

CriticalLock::~CriticalLock()
{
	vPortExitCritical(&lock.lock);
}

Critical::Critical() : lock(portMUX_INITIALIZER_UNLOCKED)
{
}

}  // namespace Ut

