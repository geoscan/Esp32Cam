//
// TaskVariant.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "TaskVariant.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

TaskVariant::~TaskVariant()
{
	switch (type) {
		case Type::Task:
			task.~Task();

			break;

		case Type::ContinuousTask:
			continuousTask.~ContinuousTask();

			break;

		case Type::Uninit:
			break;
	}
}

/// \brief The task will get re-invoked for as long as it returns true.
bool TaskVariant::operator()()
{
	switch (type) {
		case Type::Task:
			task();

			return false;

		case Type::ContinuousTask:
			return continuousTask();

		case Type::Uninit:
			return false;
	}

	return false;
}

void TaskVariant::moveImpl(TaskVariant &&aTask)
{
	switch (aTask.type) {
		case Type::Task:
			task = std::move(aTask.task);

			break;

		case Type::ContinuousTask:
			continuousTask = std::move(aTask.continuousTask);

			break;

		case Type::Uninit:
			break;
	}

	aTask.type = Type::Uninit;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
