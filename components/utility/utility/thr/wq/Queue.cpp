//
// Queue.cpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Queue.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

Queue::Queue() : queue{}
{
}

void Queue::push(TaskVariant &&aTask)
{
	std::lock_guard<std::mutex> lock{mutex};
	(void)lock;
	queue.emplace_back(std::move(aTask));
}

bool Queue::pop(TaskVariant &aTask)
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
