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
#include "utility/time.hpp"
#include <sdkconfig.h>
#include <functional>
#include <chrono>
#include <mutex>
#include <list>

namespace Utility {
namespace Thr {
namespace Wq {

using Task = std::function<void()>;  ///< Regular tasks are removed from the queue after they are invoked.
using ContinuousTask = std::function<bool()>;  ///< Continuous tasks are kept invoked iteratively for as long as they return true

/// \brief Work queue is a separate thread that receives functor objects, enqueues them for execution, picks them from the
/// queue and invokes.
///
/// \details Work queue solves the problem of overflowing stacks. Instead of allocating huge stacks for several tasks,
/// it is more efficient to run one thread with a gargantuan stack size and queue heavy-load operations from other
/// threads into it. That other threads, therefore, may possess modest or even tiny stack sizes.
///
/// Another use case for it is queueing operations which cannot be executed from a current context (e.g. when
/// interrupts are disabled, and so are some peripheral interfaces that rely on, for example UART configured for being
/// used w/ DMA, which in turn relies on interrupts being enabled).
///
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
	template <class Trep, class Tper>
	static ContinuousTask makeContinuousTimed(ContinuousTask aTask, const std::chrono::duration<Trep, Tper> &aDuration)
	{
		const auto start{Utility::bootTimeUs()};
		const auto timeout = std::chrono::duration_cast<std::chrono::microseconds>(aDuration).count();
		return [aTask, start, timeout]()
			{
				return aTask() && !Utility::expired(start, timeout);
			};
	}

	WorkQueue() : FreertosTask("WorkQueue", Istack, Iprio, Icore)
	{
	}

	/// \brief Push task into the queue
	///
	void push(Task &&aTask)
	{
		queue.push(std::move(aTask));
		resume();
	}

	/// \brief Push task into the queue and wait for it to finish for a given timespan. Returns true, if the semaphore has
	/// been released in the given time.
	///
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

	/// \brief Push task into the queue and wait for it to finish. Enables synchronized execution of tasks.
	///
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

	/// \brief Push a continuous task into the queue.
	///
	/// \details The task will be continuously polled and rescheduled for as long as it returns true
	///
	void pushContinuous(ContinuousTask &&aTask)
	{
		push([aTask]()
			{
				if (aTask()) {
					pushContinuous(aTask);
				}
			});
		resume();
	}

	/// \brief Push a continuous task into the queue and wait for it to finish
	///
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
		resume();
		sem.acquire();
	}

	/// \brief Push a continuous task into the queue and wait for it to finish for a given timespan.
	///
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
		resume();

		return sem.try_acquire_for(aTimeout);
	}

	/// \brief Run the queue
	///
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
	static Queue queue;  ///< Task storage
};

template <int Istack, int Iprio, FreertosTask::CorePin Icore>
typename WorkQueue<Istack, Iprio, Icore>::Queue WorkQueue<Istack, Iprio, Icore>::queue{};

using MediumPriority = WorkQueue<CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT + 4096, FreertosTask::PriorityMedium,
	FreertosTask::CorePin::CoreNone>;

}  // namespace Wq
}  // namespace Thr
}  // namespace Utility

#endif // UTILITY_UTILITY_WORKQUEUE_HPP_
