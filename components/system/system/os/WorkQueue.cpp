//
// WorkQueue.cpp
//
// Created on: Aug 30, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "WorkQueue.hpp"

namespace Sys {

bool WorkQueueTaskCallableVariant::callStaticWorkQueueTaskCallable()
{
	return staticWorkQueueTaskCallable(userData);
}

bool WorkQueueTaskCallableVariant::callMemberWorkQueueTaskCallable()
{
	return memberWorkQueueTaskCallable->onCalledFromWorkQueue(userData);
}

}  // Sys
