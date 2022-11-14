#include "utility/time.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <limits>

namespace Ut {

void waitMs(unsigned timeWaitMs)
{
	vTaskDelay(pdMS_TO_TICKS(timeWaitMs));
}

// Microseconds
Time bootTimeUs()
{
	return esp_timer_get_time();
}

}  // namespace Ut
