//
// Queue.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <mapbox/variant.hpp>
#include "TaskVariantQueue.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

TaskVariantQueue::TaskVariantQueue() : queue{}
{
}

void TaskVariantQueue::push(TaskVariant &&aTask)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	queue.emplace_back(std::move(aTask));
}

bool TaskVariantQueue::pop(TaskVariant &aTask)
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
