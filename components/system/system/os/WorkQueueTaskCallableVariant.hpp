//
// WorkQueueTaskCallableVariant.hpp
//
// Created on: Aug 30, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//


#ifndef COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUETASKCALLABLEVARIANT_HPP
#define COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUETASKCALLABLEVARIANT_HPP

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

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUETASKCALLABLEVARIANT_HPP
