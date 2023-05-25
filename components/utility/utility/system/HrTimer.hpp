//
// HrTimer.hpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// OO wrapper over esp_timer API
//

#ifndef UTILITY_HRTIMERWRAPPER_HPP_
#define UTILITY_HRTIMERWRAPPER_HPP_

#include <esp_timer.h>
#include <chrono>

namespace Ut {
namespace Sys {

///
/// \brief HrTimer is an OO wrapper over esp_timer API. Refer to
/// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html#_CPPv420esp_timer_dispatch_t
/// for more info
///
class HrTimer {
public:
	HrTimer(esp_timer_dispatch_t = ESP_TIMER_TASK, const char *aName = "", bool aSkipUnhandledEvents = false);
	virtual ~HrTimer();
	virtual void onHrTimer() = 0;

	template <class Rep, class Period>
	esp_err_t startOnce(const std::chrono::duration<Rep, Period> &);

	template <class Rep, class Period>
	esp_err_t startPeriodic(const std::chrono::duration<Rep, Period> &);

	inline void stopTimer()
	{
		esp_timer_stop(handle);
	}

private:
	static void onTimerStaticCallback(void *arg);

private:
	esp_timer_handle_t handle;
};

}  // namespace Sys
}  // namespace Ut

#include "HrTimer.impl"

#endif // UTILITY_HRTIMERWRAPPER_HPP_
