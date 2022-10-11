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
#include "utility/al/Time.hpp"
#include "utility/time.hpp"
#include "utility/thr/wq/Types.hpp"
#include "utility/thr/wq/Queue.hpp"
#include <sdkconfig.h>
#include <chrono>
#include <esp_log.h>

#if 0
# define WQ_DEBUG(...) ESP_LOGI("WQ", __VA_ARGS__)
#else
# define WQ_DEBUG(...)
#endif

namespace Ut {
namespace Thr {
namespace Wq {

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
public:
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
		Ut::Thr::Semaphore<1, 0> sem{};
		push([sem, aTask]() mutable
			{
				aTask();
				sem.release();
			});
		resume();
		vTaskDelay(1);
		return sem.try_acquire_for(aTimeout);
	}

	/// \brief Push task into the queue and wait for it to finish. Enables synchronized execution of tasks.
	///
	void pushWait(Task &&aTask)
	{
		Ut::Thr::Semaphore<1, 0> sem{};
		push([&sem, &aTask]()
			{
				aTask();
				sem.release();
			});
		resume();
		vTaskDelay(1);
		sem.acquire();
	}

	/// \brief Push a continuous task into the queue.
	///
	/// \details The task will be continuously polled and rescheduled for as long as it returns true
	///
	void pushContinuous(ContinuousTask &&aTask)
	{
		push([this, aTask]() mutable
			{
				if (aTask()) {
					WQ_DEBUG("pushContinuous() re-pushing the task");
					pushContinuous(std::move(aTask));
				} else {
					WQ_DEBUG("pushContinuous() discontinuing the task");
				}
			});
		resume();
	}

	/// \brief Push a continuous task into the queue and wait for it to finish
	///
	void pushContinuousWait(ContinuousTask &&aTask)
	{
		Ut::Thr::Semaphore<1, 0> sem;  // Initialize a busy semaphore
		pushContinuous([&sem, &aTask]()
			{
				WQ_DEBUG("pushContinuousWait() invoking");
				bool f = aTask();

				if (!f) {
					WQ_DEBUG("pushContinuousWait() releasing sem.");
					sem.release();
				}

				return f;
			});
		resume();
		vTaskDelay(1);
		sem.acquire();
	}

	/// \brief Push a continuous task into the queue and wait for it to finish for a given timespan.
	///
	template <class Trep, class Tper>
	bool pushContinuousWaitFor(ContinuousTask &&aTask, const std::chrono::duration<Trep, Tper> &aTimeout)
	{
		Ut::Thr::Semaphore<1, 0> sem;  // Initialize a busy semaphore
		pushContinuous([sem, aTask]() mutable
			{
				bool f = aTask();

				if (!f) {
					sem.release();
				}

				return f;
			});
		resume();
		vTaskDelay(1);

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
	Queue queue;  ///< Task storage
};

using MediumPriority = WorkQueue<CONFIG_PTHREAD_TASK_STACK_SIZE_DEFAULT + 7000, FreertosTask::PriorityMedium,
	FreertosTask::CorePin::Core1>;

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#undef WQ_DEBUG

#endif // UTILITY_UTILITY_WORKQUEUE_HPP_
