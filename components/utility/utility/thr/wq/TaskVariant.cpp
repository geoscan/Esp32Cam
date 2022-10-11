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

TaskVariant::TaskVariant(ContinuousTask &&aTask) : type{Type::ContinuousTask}
{
	new (storage.storage) ContinuousTask{aTask};
}

TaskVariant::TaskVariant(Task &&aTask) : type{Type::Task}
{
	new (storage.storage) Task{aTask};
}

TaskVariant::~TaskVariant()
{
	destructImpl();
}

/// \brief The task will get re-invoked for as long as it returns true.
bool TaskVariant::operator()()
{
	switch (type) {
		case Type::Task:
			storage.asTask()();

			return false;

		case Type::ContinuousTask:
			return storage.asContinuousTask()();

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
	std::copy_n(aTask.storage.storage, sizeof(storage.storage), storage.storage);
	aTask.type = Type::Uninit;
}

void TaskVariant::destructImpl()
{
	switch (type) {
		case Type::Task:
			storage.asTask().~Task();

			break;

		case Type::ContinuousTask:
			storage.asContinuousTask().~ContinuousTask();

			break;

		case Type::Uninit:
			break;
	}

	type = Type::Uninit;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
