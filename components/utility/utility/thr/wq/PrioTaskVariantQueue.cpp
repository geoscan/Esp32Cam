//
// PrioTaskVariantQueue.cpp
//
// Created on: Oct 12, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <mapbox/variant.hpp>
#include "utility/thr/wq/TaskVariant.hpp"
#include "utility/thr/wq/TaskVariantQueue.hpp"
#include "PrioTaskVariantQueue.hpp"

namespace Ut {
namespace Thr {
namespace Wq {

void PrioTaskVariantQueue::push(TaskVariant &&aTaskVariant)
{
	queues[static_cast<std::size_t>(aTaskVariant.priority())].push(std::move(aTaskVariant));
}

bool PrioTaskVariantQueue::pop(TaskVariant &aTaskVariant)
{
	for (auto &queue : queues) {
		if (queue.pop(aTaskVariant)) {
			return true;
		}
	}

	return false;
}

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut
