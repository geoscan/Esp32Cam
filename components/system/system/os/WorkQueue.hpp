//
// WorkQueue.hpp
//
// Created on: Aug 30, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//


#ifndef COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUE_HPP
#define COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUE_HPP

namespace Sys {

/// Returns true when should be placed back into the queue after invoked
using StaticWorkQueueTaskCallable = bool(*)(void *aUserData);

class MemberWorkQueueTaskCallable {
public:
	/// \brief Returns true if should be placed back into the queue after
	/// invoked
	virtual bool onCalledFromWorkQueue(void *aUserData) = 0;
	virtual ~MemberWorkQueueTaskCallable() = default;
};

/// Encapsulates various types of work queue callables under a convenient
/// polymorphic interface
class WorkQueueTaskCallableVariant final {
private:
	bool callStaticWorkQueueTaskCallable();
	bool callMemberWorkQueueTaskCallable();

private:
	union {
		StaticWorkQueueTaskCallable staticWorkQueueTaskCallable;
		MemberWorkQueueTaskCallable *memberWorkQueueTaskCallable;
	};
	void *userData;
	bool (WorkQueueTaskCallableVariant:: *virtualCallable)();

public:
	WorkQueueTaskCallableVariant(const WorkQueueTaskCallableVariant &) = default;
	WorkQueueTaskCallableVariant(WorkQueueTaskCallableVariant &&) = default;
	WorkQueueTaskCallableVariant &operator=(const WorkQueueTaskCallableVariant &) = default;
	WorkQueueTaskCallableVariant &operator=(WorkQueueTaskCallableVariant &&) = default;

	inline WorkQueueTaskCallableVariant(StaticWorkQueueTaskCallable aStaticWorkQueueTaskCallable,
		void *aUserData = nullptr):
		staticWorkQueueTaskCallable{aStaticWorkQueueTaskCallable},
		userData{aUserData},
		virtualCallable{&WorkQueueTaskCallableVariant::callStaticWorkQueueTaskCallable}
	{
	}

	inline WorkQueueTaskCallableVariant(MemberWorkQueueTaskCallable *aMemberWorkQueueTaskCallable,
		void *aUserData = nullptr):
		memberWorkQueueTaskCallable{aMemberWorkQueueTaskCallable},
		userData{aUserData},
		virtualCallable{&WorkQueueTaskCallableVariant::callStaticWorkQueueTaskCallable}
	{
	}

	bool invokeTask()
	{
		return (this->*virtualCallable)();
	}
};

/// \brief A separate process that is responsible for continuous implementation
/// of various tasks such as event handlers, and other.
class WorkQueue {
public:
	virtual void pushTask(const WorkQueueTaskCallableVariant &aWorkQueueTaskCallableVariant) = 0;
	virtual ~WorkQueue() = default;

	inline void pushTask(MemberWorkQueueTaskCallable *aMemberWorkQueueTaskCallable, void *aUserArgument = nullptr)
	{
		pushTask({aMemberWorkQueueTaskCallable, aUserArgument});
	}

	inline void pushTask(StaticWorkQueueTaskCallable aStaticWorkQueueTaskCallable, void *aUserArgument = nullptr)
	{
		pushTask({aStaticWorkQueueTaskCallable, aUserArgument});
	}
};

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUE_HPP
