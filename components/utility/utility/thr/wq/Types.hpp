//
// Types.hpp
//
// Created on: Oct 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_THR_WQ_TYPES_HPP_)
#define UTILITY_UTILITY_THR_WQ_TYPES_HPP_

#include <functional>

namespace Ut {
namespace Thr {
namespace Wq {

using Task = std::function<void()>;  ///< Regular tasks are removed from the queue after they are invoked.
using ContinuousTask = std::function<bool()>;  ///< Continuous tasks are kept invoked iteratively for as long as they return true

/// \brief Named enumeration that establishes relative priorities b/w tasks
enum class TaskPrio {
	Lowest = 0,
	Default,
	Tracker,
};

}  // namespace Wq
}  // namespace Thr
}  // namespace Ut

#endif // UTILITY_UTILITY_THR_WQ_TYPES_HPP_
