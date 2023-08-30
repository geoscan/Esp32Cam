//
// WorkQueue.hpp
//
// Created on: Aug 30, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//


#ifndef COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUE_HPP
#define COMPONENTS_SYSTEM_SYSTEM_OS_WORKQUEUE_HPP

#include "system/os/WorkQueueTaskCallableVariant.hpp"

namespace Sys {

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
