#ifndef COMPONENTS_UTILITY_TIME_HPP
#define COMPONENTS_UTILITY_TIME_HPP

#include <esp_timer.h>

namespace Utility {

using Time = int64_t;

// Wait for a certain duration
void waitMs(unsigned timeWaitMs);

// Microseconds since boot
Time bootTimeUs();

// Check if required time period has already passed
bool expired(const Time sinceUs, Time periodUs);

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_TIME_HPP
