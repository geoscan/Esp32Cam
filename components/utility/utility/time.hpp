#ifndef COMPONENTS_UTILITY_TIME_HPP
#define COMPONENTS_UTILITY_TIME_HPP

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

namespace Utility {

void waitMs(unsigned timeWaitMs)
{
	vTaskDelay((timeWaitMs) / portTICK_PERIOD_MS);
}

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_TIME_HPP
