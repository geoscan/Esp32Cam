//
// HrTimer.cpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/HrTimer.hpp"

namespace Utility {
namespace Tim {

HrTimer::HrTimer(esp_timer_dispatch_t aDispatchMethod, const char *aName, bool aSkipUnhandledEvents)
{
	esp_timer_create_args_t args {
		HrTimer::onTimerStaticCallback,
		this,
		aDispatchMethod,
		aName,
		aSkipUnhandledEvents
	};
	esp_timer_create(&args, &handle);
}

HrTimer::~HrTimer()
{
	esp_timer_stop(handle);
	esp_timer_delete(handle);
}

void HrTimer::onTimerStaticCallback(void *aInstance)
{
	static_cast<HrTimer *>(aInstance)->onHrTimer();
}

}  // namespace Tim
}  // namespace Utility
