//
// WorkQueue.hpp
//
// Created on: Jun 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_WORKQUEUE_HPP_)
#define UTILITY_UTILITY_WORKQUEUE_HPP_

#include "utility/Semaphore.hpp"
#include "utility/MakeSingleton.hpp"
#include "utility/Threading.hpp"
#include <sdkconfig.h>
#include <functional>
#include <chrono>
#include <mutex>
#include <list>

namespace Utility {
namespace Threading {
namespace Wq {

using Task = std::function<void()>;

template <int Istack = CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT, FreertosTask::Prioroty Iprio, FreertosTask::CorePin Icore>
class WorkQueue : public MakeSingleton<WorkQueue<Istack, Iprio>>, public FreertosTask {
private:
	struct Queue {
		using QueueType = std::list<Task>;
		QueueType queue;
		std::mutex mutex;

		void push(Task &&aTask)
		{
			std::lock_guard<std::mutex> lock{mutex};
			(void)lock;
			queue.push_back(std::move(aTask));
		}

		bool pop(Task &aTask)
		{
			std::lock_guard<std::mutex> lock{mutex};
			(void)lock;
			bool ret = queue.empty();

			if (ret) {
				aTask = std::move(queue.front());
				queue.pop_front();
			}

			return ret;
		}
	};

public:
	WorkQueue() : FreertosTask("WorkQueue", Istack, Iprio, Icore)
	{
	}

	void push(Task &&aTask)
	{
		queue.push(std::move(aTask));
		resume();
	}

	template <class Trep, class Tper>
	bool pushWaitFor(Task &&aTask, const std::chrono::duration<Trep, Tper> &aTimeout)
	{
		Utility::Semaphore<1, 0> sem{};
		push([&sem, aTask]()
			{
				aTask();
				sem.release();
			});
		resume();
		return sem.try_acquire_for(aTimeout);
	}

	void pushWait(Task &&aTask)
	{
		Utility::Semaphore<1, 0> sem{};
		push([&sem, aTask]()
			{
				aTask();
				sem.release();
			});
		resume();
		sem.acquire();
	}

	void run() override
	{
		while (true) {
			Task task;
			if (queue.pop(task)) {
				task();
			} else {
				suspend();
			}
		}
	}

private:
	static Queue queue;
};

using MediumPriority = WorkQueue<CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT, tskIDLE_PRIORITY, -1>;

}  // namespace Wq
}  // namespace Threading
}  // namespace Utility

#endif // UTILITY_UTILITY_WORKQUEUE_HPP_
