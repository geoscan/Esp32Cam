//
// TaskVariant.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "TaskVariant.hpp"
#include <cassert>
#include <algorithm>
#include <esp_log.h>

namespace Ut {
namespace Thr {
namespace Wq {

TaskVariant::TaskVariant(Task &&aTask, TaskPrio aPrio) : task{std::move(aTask)}, prio{aPrio}
{
}

TaskVariant::~TaskVariant()
{
}

/// \brief The task will get re-invoked for as long as it returns true.
bool TaskVariant::operator()()
{
	task();

	return false;
}

void TaskVariant::moveImpl(TaskVariant &&aTask)
{
	task = std::move(aTask.task);
	prio = aTask.prio;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
