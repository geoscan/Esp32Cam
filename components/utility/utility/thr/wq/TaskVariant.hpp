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
private:
	union Variant {
		Task task;
		ContinuousTask continuousTask;
	};
	enum class Type {
		Uninit,
		Task,
		ContinuousTask,
	};
	struct Storage {
		alignas(long int) std::uint8_t storage[sizeof(Variant)];

		inline Task &asTask()
		{
			return *reinterpret_cast<Task *>(storage);
		}
		inline ContinuousTask &asContinuousTask()
		{
			return *reinterpret_cast<ContinuousTask *>(storage);
		}
	};
public:
	inline TaskVariant() : type{Type::Uninit}
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
	inline bool isValid() const
	{
		return type != Type::Uninit;
	}

	TaskVariant(ContinuousTask &&aTask);
	TaskVariant(Task &&aTask);
	TaskVariant(const TaskVariant &) = delete;
	TaskVariant &operator=(const TaskVariant &) = delete;
	~TaskVariant();
	bool operator()();
private:
	void moveImpl(TaskVariant &&);
	void destructImpl();
private:
private:
	Type type;
	alignas(long int) Storage storage;
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_TASKVARIANT_HPP_
