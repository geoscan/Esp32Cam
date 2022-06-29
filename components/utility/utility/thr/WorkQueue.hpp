//
// WorkQueue.hpp
//
// Created on: Jun 03, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_WORKQUEUE_HPP_)
#define UTILITY_UTILITY_WORKQUEUE_HPP_

#include "utility/thr/Semaphore.hpp"
#include "utility/MakeSingleton.hpp"
#include "utility/thr/Threading.hpp"
#include <sdkconfig.h>
#include <functional>
#include <chrono>
#include <mutex>
#include <list>

namespace Utility {
namespace Thr {
namespace Wq {

using Task = std::function<void()>;
using ContinuousTask = std::function<bool()>;  ///< Continuous tasks are kept invoked iteratively for as long as they return true

template <int Istack, int Iprio, FreertosTask::CorePin Icore>
class WorkQueue : public MakeSingleton<WorkQueue<Istack, Iprio, Icore>>, public FreertosTask {
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
			bool ret = !queue.empty();

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
		Utility::Thr::Semaphore<1, 0> sem{};
		push([sem, aTask]() mutable
			{
				aTask();
				sem.release();
			});
		resume();
		return sem.try_acquire_for(aTimeout);
	}

	void pushWait(Task &&aTask)
	{
		Utility::Thr::Semaphore<1, 0> sem{};
		push([&sem, &aTask]()
			{
				aTask();
				sem.release();
			});
		resume();
		sem.acquire();
	}

	void pushContinuous(ContinuousTask &&aTask)
	{
		push([aTask]()
			{
				if (aTask()) {
					pushContinuous(aTask);
				}
			});
	}

	void pushContinuousWait(ContinuousTask &&aTask)
	{
		Utility::Thr::Semaphore<1, 0> sem;  // Initialize a busy semaphore
		pushContinuous([&sem, &aTask]()
			{
				bool f = aTask();

				if (!f) {
					sem.release();
				}

				return f;
			});

		sem.acquire();
	}

	template <class Trep, class Tper>
	bool pushContinuousWaitFor(ContinuousTask &&aTask, const std::chrono::duration<Trep, Tper> &aTimeout)
	{
		Utility::Thr::Semaphore<1, 0> sem;  // Initialize a busy semaphore
		pushContinuous([sem, aTask]() mutable
			{
				bool f = aTask();

				if (!f) {
					sem.release();
				}

				return f;
			});

		return sem.try_acquire_for(aTimeout);
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

template <int Istack, int Iprio, FreertosTask::CorePin Icore>
typename WorkQueue<Istack, Iprio, Icore>::Queue WorkQueue<Istack, Iprio, Icore>::queue{};

using MediumPriority = WorkQueue<CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT + 4096, FreertosTask::PriorityMedium,
	FreertosTask::CorePin::CoreNone>;

}  // namespace Wq
}  // namespace Thr
}  // namespace Utility

#endif // UTILITY_UTILITY_WORKQUEUE_HPP_
