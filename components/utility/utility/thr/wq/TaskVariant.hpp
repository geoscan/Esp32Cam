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

class TaskVariant {
private:
	enum class Type {
		Uninit,
		Task,
		ContinuousTask,
	};
public:
	inline TaskVariant(TaskVariant &&aTask)
	{
		moveImpl(std::move(aTask));
	}
	inline TaskVariant &operator=(TaskVariant &&aTask)
	{
		moveImpl(std::move(aTask));

		return *this;
	}
	inline TaskVariant(ContinuousTask &&aTask) : type{Type::ContinuousTask}, continuousTask{std::move(aTask)}
	{
	}
	inline TaskVariant(Task &&aTask) : type{Type::Task}, task{std::move(aTask)}
	{
	}

	TaskVariant(const TaskVariant &) = delete;
	TaskVariant &operator=(const TaskVariant &) = delete;
	~TaskVariant();
	bool operator()();
private:
	void moveImpl(TaskVariant &&);
private:
	Type type;
	union {
		Task task;
		ContinuousTask continuousTask;
	};
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_
