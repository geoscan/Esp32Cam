//
// TaskQueue.hpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_THR_WQ_TASK_QUEUE)
#define UTILITY_UTILITY_THR_WQ_TASK_QUEUE

#include "utility/thr/wq/Types.hpp"
#include <mutex>
#include <deque>

namespace Ut {
namespace Thr {
namespace Wq {

class TaskQueue {
public:
	TaskQueue();
	void push(Task &&aTask);
	bool pop(Task &aTask);
private:
	using TaskQueueType = std::deque<Task>;
	TaskQueueType queue;
	std::mutex mutex;
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_TASK_QUEUE
