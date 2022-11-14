//
// TaskVariant.hpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_)
#define UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_

#include "utility/thr/wq/Types.hpp"

namespace mapbox {
namespace util {

template <class ...Ts>
class variant;

}  // namespace util
}  // namespace mapbox

namespace Ut {
namespace Thr {
namespace Wq {

class TaskVariant final {
private:
	using Variant = mapbox::util::variant<Task, ContinuousTask>;
public:
	inline TaskVariant() : prio{TaskPrio::Lowest}
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
	TaskVariant(ContinuousTask &&aContinuousTask, TaskPrio aPrio);
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
	Variant variant;
	TaskPrio prio;
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

namespace std {

bool operator<(const Ut::Thr::Wq::TaskVariant &, const Ut::Thr::Wq::TaskVariant &);

}  // namespace std

#endif // UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_
