//
// Time.cpp
//
// Created on: Aug 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Time.hpp"

namespace Ut {
namespace Al {

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

}  // namespace Al
}  // namespace Ut
