//
// TaskVariant.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <cassert>
#include <algorithm>
#include <esp_log.h>
#include <mapbox/variant.hpp>
#include "TaskVariant.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

TaskVariant::TaskVariant(Task &&aTask, TaskPrio aPrio) : variant{std::move(aTask)}, prio{aPrio}
{
}

TaskVariant::TaskVariant(ContinuousTask &&aContinuousTask, TaskPrio aPrio) : variant{std::move(aContinuousTask)},
	prio{aPrio}
{
}

TaskVariant::~TaskVariant()
{
}

/// \brief The task will get re-invoked for as long as it returns true.
bool TaskVariant::operator()()
{
	return variant.match(
		[](Task &aTask)
		{
			aTask();
			return false;
		},
		[](ContinuousTask &aContinuousTask) { return aContinuousTask(); });
}

void TaskVariant::moveImpl(TaskVariant &&aTask)
{
	variant = std::move(aTask.variant);
	prio = aTask.prio;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

bool std::operator<(const Ut::Thr::Wq::TaskVariant &aLhs, const Ut::Thr::Wq::TaskVariant &aRhs)
{
	return static_cast<int>(aLhs.priority()) < static_cast<int>(aRhs.priority());
}
