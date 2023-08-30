//
// PortWorkQueue.cpp
//
// Created on: Aug 30, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "system/os/Logger.hpp"
#include "utility/thr/WorkQueue.hpp"
#include "wq/wq.hpp"
#include <cassert>

#include "PortWorkQueue.hpp"

namespace Wq {

static constexpr const char *kLogPreamble = "PortWorkQueue";

void PortWorkQueue::pushTask(const Sys::WorkQueueTaskCallableVariant &aWorkQueueTaskCallableVariant)
{
	if (!Ut::Thr::Wq::MediumPriority::checkInstance()) {
		Sys::Logger::write(Sys::LogLevel::Error, Wq::debugTag(),
			"Failed to access an instance of a WorkQueue, panicking");
		assert(false);
	}

	Ut::Thr::Wq::MediumPriority::getInstance().pushContinuous(
		[aWorkQueueTaskCallableVariant]()
		{
			return aWorkQueueTaskCallableVariant.invokeTask();
		});
}

}  // Wq
