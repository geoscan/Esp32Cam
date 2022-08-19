//
// CriticalLock.hpp
//
// Created on: Mar 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_CRITICALLOCK_HPP
#define UTILITY_UTILITY_CRITICALLOCK_HPP

#include <freertos/FreeRTOS.h>

namespace Ut {

class Critical final {
private:
	friend class CriticalLock;
	portMUX_TYPE lock;

public:
	Critical();
};

class CriticalLock {
public:
	CriticalLock(Critical &);
	CriticalLock(const CriticalLock &) = delete;
	CriticalLock(CriticalLock &&) = delete;
	CriticalLock &operator=(const CriticalLock &) = delete;
	CriticalLock &operator=(CriticalLock &&) = delete;
	~CriticalLock();
private:
	Critical &lock;
};

}  // namespace Ut

#endif // UTILITY_UTILITY_CRITICALLOCK_HPP
