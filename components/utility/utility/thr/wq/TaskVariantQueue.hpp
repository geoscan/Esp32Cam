//
// TaskVariantQueue.hpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_THR_WQ_TASKVARIANTQUEUE)
#define UTILITY_UTILITY_THR_WQ_TASKVARIANTQUEUE

#include "utility/thr/wq/Types.hpp"
#include "utility/thr/wq/TaskVariant.hpp"
#include <mutex>
#include <deque>

namespace Ut {
namespace Thr {
namespace Wq {

class TaskVariantQueue {
public:
	TaskVariantQueue();
	void push(TaskVariant &&aTask);
	bool pop(TaskVariant &aTask);
private:
	using QueueType = std::deque<TaskVariant>;
	QueueType queue;
	std::mutex mutex;
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_TASKVARIANTQUEUE
