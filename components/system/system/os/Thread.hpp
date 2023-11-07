//
// Thread.hpp
//
// Created on: Oct 24, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef SYSTEM_SYSTEM_OS_THREAD_HPP
#define SYSTEM_SYSTEM_OS_THREAD_HPP

#include <cstdint>

namespace Sys {

/// \brief Implements basic thread / task-related functionality
class Thread {
public:
	/// \brief Implements delay functionality for the current task
	///
	/// \details
	/// - The control MAY BE yelded to a pending task.
	/// - The actual delay MAY exceed the `aMilliseconds` value
	/// - The delay is GUARANTEED to be >= `aMilliseconds`
	static void delayMs(std::size_t aMilliseconds);
};

}  // Sys

#endif // SYSTEM_SYSTEM_OS_THREAD_HPP
