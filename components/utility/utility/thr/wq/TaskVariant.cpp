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

TaskVariant::TaskVariant(Task &&aTask) : task{std::move(aTask)}
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
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
