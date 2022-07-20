//
// Semaphore.hpp
//
// Created on: Mar 31, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_SEMAPHORE_HPP
#define UTILITY_UTILITY_SEMAPHORE_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <type_traits>
#include <memory>
#include <chrono>

namespace Utility {
namespace Thr {

template <std::size_t Slots, std::size_t FreeSlots = Slots>
class Semaphore final {
	static_assert(Slots >= FreeSlots, "[Semaphore] Number of FreeSlots exceeds number of available Slots");
	static_assert(Slots > 0, "[Semaphore] Number of slots must be positive");

	std::shared_ptr<std::remove_pointer<SemaphoreHandle_t>::type> sem;
public:
	// STL-compliant methods
	// https://en.cppreference.com/w/cpp/thread/counting_semaphore
	constexpr std::ptrdiff_t max() const;
	void acquire();
	void release();
	bool try_acquire();

	template <typename Rep, typename Period>
	bool try_acquire_for(const std::chrono::duration<Rep, Period> &);

	template <typename Clock, typename Duration>
	bool try_acquire_until(const std::chrono::time_point<Clock, Duration> &);

	Semaphore();
	~Semaphore() = default;

	Semaphore(const Semaphore &) = default;
	Semaphore(Semaphore &&) = default;

	Semaphore &operator=(const Semaphore &) = default;
	Semaphore &operator=(Semaphore &&) = default;
};

template <class T>
class SemaphoreLock final {
public:
	SemaphoreLock(T &aSem) : sem{aSem}
	{
		sem.acquire();
	}

	~SemaphoreLock()
	{
		sem.release();
	}

	SemaphoreLock(const SemaphoreLock &) = delete;
	SemaphoreLock(SemaphoreLock &&) = delete;
	SemaphoreLock &operator=(const SemaphoreLock &) = delete;
	SemaphoreLock &operator=(SemaphoreLock &&) = delete;
private:
	T& sem;
};

}  // namespace Thr
}  // namespace Utility

#include "Semaphore.impl"

#endif  // UTILITY_UTILITY_SEMAPHORE_HPP
