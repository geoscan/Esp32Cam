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
	ESP_LOGI("Critical", "Enter");
	vTaskEnterCritical(&lock.lock);
}

CriticalLock::~CriticalLock()
{
	vTaskExitCritical(&lock.lock);
	ESP_LOGI("Critical", "Exit");
//	portEXIT_CRITICAL(&lock.lock);
}

Critical::Critical() : lock(portMUX_INITIALIZER_UNLOCKED)
{
}

}  // namespace Utility

