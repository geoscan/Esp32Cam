#include "utility/time.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <limits>

namespace Utility {

void waitMs(unsigned timeWaitMs)
{
	vTaskDelay((timeWaitMs) / portTICK_PERIOD_MS);
}

// Microseconds
Time bootTimeUs()
{
	return esp_timer_get_time();
}

bool expired(const Time time, Time durationUs)
{
	bool       isExpired;
	const Time now    = esp_timer_get_time();
	const Time passed = (now < time) ? /*then*/ std::numeric_limits<Time>::max() - time +
		now - std::numeric_limits<Time>::min() : /*else*/ now - time;  // Considering overflow

	if (passed > durationUs) {
		isExpired = true;
	} else {
		isExpired = false;
	}

	return isExpired;
}

}  // namespace Utility