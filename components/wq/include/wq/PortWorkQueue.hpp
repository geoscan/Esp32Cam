//
// PortWorkQueue.hpp
//
// Created on: Aug 30, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_WQ_WQ_PORTWORKQUEUE_HPP
#define COMPONENTS_WQ_WQ_PORTWORKQUEUE_HPP

#include "system/os/WorkQueue.hpp"

namespace Wq {

class PortWorkQueue : public Sys::WorkQueue {
public:
	void pushTask(const Sys::WorkQueueTaskCallableVariant &aWorkQueueTaskCallableVariant) override;
};

}  // Wq

#endif // COMPONENTS_WQ_WQ_PORTWORKQUEUE_HPP
