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

bool expired(const Time sinceUs, Time durationUs)
{
	bool       isExpired;
	const Time now    = bootTimeUs();
	const Time passed = (now < sinceUs) ? /*then*/ std::numeric_limits<Time>::max() - sinceUs +
		now - std::numeric_limits<Time>::min() : /*else*/ now - sinceUs;  // Considering overflow

	if (passed > durationUs) {
		isExpired = true;
	} else {
		isExpired = false;
	}

	return isExpired;
}

}  // namespace Utility