//
// PrioTaskVariantQueue.hpp
//
// Created on: Oct 12, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_THR_WQ_PRIOTASKVARIANTQUEUE_HPP_)
#define UTILITY_UTILITY_THR_WQ_PRIOTASKVARIANTQUEUE_HPP_

#include "utility/thr/wq/Types.hpp"
#include <array>

namespace Ut {
namespace Thr {
namespace Wq {

class TaskVariantQueue;
class TaskVariant;

class PrioTaskVariantQueue {
public:
	void push(TaskVariant &&aTaskVariant);
	bool pop(TaskVariant &aTaskVariant);
private:
	std::array<TaskVariantQueue, static_cast<std::size_t>(TaskPrio::N)> queues;
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_PRIOTASKVARIANTQUEUE_HPP_
