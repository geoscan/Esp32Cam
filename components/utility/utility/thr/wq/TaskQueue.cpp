//
// TaskQueue.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "TaskQueue.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

TaskQueue::TaskQueue() : queue(32)
{
	queue.clear();
}

void TaskQueue::push(Task &&aTask)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	queue.push_back(aTask);
}

bool TaskQueue::pop(Task &aTask)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	bool ret = !queue.empty();

	if (ret) {
		aTask = std::move(queue.front());
		queue.pop_front();
	}

	return ret;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
