//
// Mutex.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_SYSTEM_SYSTEM_OS_MUTEX_HPP
#define COMPONENTS_SYSTEM_SYSTEM_OS_MUTEX_HPP

#include <mutex>

namespace Sys {

/// \brief Platform-agnostic mutex functionality implementor
class Mutex : private std::mutex {
public:
	using std::mutex::lock;
	using std::mutex::unlock;
	using std::mutex::mutex;
};

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_OS_MUTEX_HPP
