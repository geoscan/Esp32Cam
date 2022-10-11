//
// TaskVariant.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "TaskVariant.hpp"
#include <cassert>
#include <esp_log.h>

namespace Ut {
namespace Thr {
namespace Wq {

TaskVariant::~TaskVariant()
{
	destructImpl();
}

/// \brief The task will get re-invoked for as long as it returns true.
bool TaskVariant::operator()()
{
	switch (type) {
		case Type::Task:
			assert(static_cast<bool>(task));
			task();

			return false;

		case Type::ContinuousTask:
			assert(static_cast<bool>(continuousTask));
			return continuousTask();

		case Type::Uninit:
			return false;

		default:
			return false;
	}
}

void TaskVariant::moveImpl(TaskVariant &&aTask)
{
	destructImpl();
	type = aTask.type;

	switch (aTask.type) {
		case Type::Task:
			assert(static_cast<bool>(aTask.task));
			task.swap(aTask.task);

			break;

		case Type::ContinuousTask:
			assert(static_cast<bool>(aTask.continuousTask));
			continuousTask.swap(aTask.continuousTask);

			break;

		case Type::Uninit:
			ESP_LOGW("WQ", "TaskVariant: move-constructor invoked upon uninitialized variant");
			break;
	}

	aTask.type = Type::Uninit;
}

void TaskVariant::destructImpl()
{
	switch (type) {
		case Type::Task:
			assert(static_cast<bool>(task));
			task = Task{};

			break;

		case Type::ContinuousTask:
			assert(static_cast<bool>(continuousTask));
			continuousTask = ContinuousTask{};

			break;

		case Type::Uninit:
			break;
	}

	type = Type::Uninit;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
