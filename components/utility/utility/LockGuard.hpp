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

//
// STL-style RAII lock guard
//

namespace Utility {

//  -----  LockGuard  -----  //

template <typename MutexType>
class LockGuard final {
private:
	std::reference_wrapper<MutexType> mutex;
	bool shouldUnlock = true;

	void setLock(bool stateLock)
	{
		if (stateLock) {
			mutex.get().lock();
		} else {
			mutex.get().unlock();
		}
	}
public:
	LockGuard() = delete;
	LockGuard(const LockGuard<MutexType> &) = delete;
	LockGuard &operator=(const LockGuard<MutexType> &) = delete;

	LockGuard(LockGuard<MutexType> &&lg) : mutex(lg.mutex) {
		lg.shouldUnlock = false;
	}

	LockGuard &operator=(LockGuard<MutexType> &&lg)
	{
		mutex = lg.mutex;
		lg.shouldUnlock = false;
		return *this;
	}

	LockGuard(MutexType &m) : mutex(std::ref(m))
	{
		setLock(true);
	}

	~LockGuard()
	{
		if (shouldUnlock) {
			setLock(false);
		}
	}
};


// ------------ makeLockGuard ------------ //


template <typename MutexType>
inline LockGuard<MutexType> makeLockGuard(MutexType &mutex)
{
	return LockGuard<MutexType>(mutex);
}

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_UTILITY_LOCKGUARD_HPP
