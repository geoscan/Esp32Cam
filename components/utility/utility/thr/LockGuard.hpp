//
// LockGuard.hpp
//
// Created on:  Sep 23, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_UTILITY_UTILITY_LOCKGUARD_HPP
#define COMPONENTS_UTILITY_UTILITY_LOCKGUARD_HPP

#include <asio.hpp>
#include <functional>

namespace Ut {

template <typename MutexType>
class LockGuard final {
private:
	MutexType &mutex;

public:
	LockGuard() = delete;
	LockGuard(const LockGuard &) = delete;
	LockGuard(LockGuard &&) = delete;
	LockGuard &operator=(const LockGuard &) = delete;
	LockGuard &operator=(LockGuard &&) = delete;

	LockGuard(MutexType &aMutex):
		mutex{aMutex}
	{
		mutex.lock();
	}

	~LockGuard()
	{
		mutex.unlock();
	}
};

template <typename MutexType>
inline LockGuard<MutexType> makeLockGuard(MutexType &mutex)
{
	return LockGuard<MutexType>(mutex);
}

}  // namespace Ut

#endif  // COMPONENTS_UTILITY_UTILITY_LOCKGUARD_HPP
