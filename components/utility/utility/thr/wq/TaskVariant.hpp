//
// TaskVariant.hpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_)
#define UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_

#include "utility/thr/wq/Types.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

class TaskVariant final {
public:
	inline TaskVariant()
	{
	}
	inline TaskVariant(TaskVariant &&aTask)
	{
		moveImpl(std::move(aTask));
	}
	inline TaskVariant &operator=(TaskVariant &&aTask)
	{
		moveImpl(std::move(aTask));

		return *this;
	}

	TaskVariant(Task &&aTask, TaskPrio aPrio);
	TaskVariant(const TaskVariant &) = delete;
	TaskVariant &operator=(const TaskVariant &) = delete;
	~TaskVariant();
	bool operator()();
	inline TaskPrio priority() const
	{
		return prio;
	}
private:
	void moveImpl(TaskVariant &&);
private:
	Task task;
	TaskPrio prio;
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_
