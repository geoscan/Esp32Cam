#ifndef COMPONENTS_UTILITY_TIME_HPP
#define COMPONENTS_UTILITY_TIME_HPP

#include <esp_timer.h>
#include <chrono>

namespace Ut {
using Time = int64_t;

// Wait for a certain duration
void waitMs(unsigned timeWaitMs);

// Microseconds since boot
Time bootTimeUs();

// Check if required time period has already passed
bool expired(const Time sinceUs, Time periodUs);

namespace Tim {

template <class Trep, class Tper>
void taskDelay(const std::chrono::duration<Trep, Tper> &aDuration)
{
	waitMs(std::chrono::duration_cast<std::chrono::milliseconds>(aDuration).count());
}

}  // namespace Tim
}  // namespace Ut

#endif  // COMPONENTS_UTILITY_TIME_HPP
