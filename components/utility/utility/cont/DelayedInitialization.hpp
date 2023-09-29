//
// DelayedInitialization.hpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMONENTS_UTILITY_UTILITY_CONT_DELAYEDINITIALIZATION_DELAYEDINITIALIZATION_HPP
#define COMONENTS_UTILITY_UTILITY_CONT_DELAYEDINITIALIZATION_DELAYEDINITIALIZATION_HPP

#include <cstdint>
#include <new>
#include <type_traits>

namespace Ut {
namespace Cont {

/// \brief Solves the problem of unpredictable order of static initialization.
///
/// Use case example:
///
/// ```
/// static DelayedInitialization<int> delayedInitialization{};
/// delayedInitialization.initialize(42);
/// delayedInitialization.getInstance() = 42;
/// ```
template <class T, class RawArrayType = std::size_t /* `std::size_t` to account for possible unaligned access issues */>
struct DelayedInitialization {
	template <class RawArrayType = T>
	constexpr calculateRawArraySize()
	{
		return sizeof(T) / sizeof(RawArrayType)
			+ (/* Still not enough memory? */ sizeof(T) % sizeof(RawArrayType)? 1 : 0);
	}

	/// \brief Memory cell
	RawArrayType rawMemory[calculateRawArraySize()] = {0};

	/// \brief Serves 2 goals: (1) as a flag that the instance is initialized,
	/// and (2) as the actual pointer, for the case `new(...)` implementation
	/// does not place the instance at the beginning of the memory.
	T *instance = nullptr;

	T *getInstance()
	{
		return instance;
	}

	bool isInitialized()
	{
		return instance != nullptr;
	}

	/// \brief Initializes a new instance in the memory.
	/// \warning Does not chech whether the instance has been initialized
	/// previously
	template <class ...Ts>
	void initialize(Ts &&...aArgs)
	{
		instance = new(static_cast<void *>(&rawMemory[0])) T{std::forward<Ts>(aArgs)...};
	}
};

}  // Cont
}  // Ut

#endif // COMONENTS_UTILITY_UTILITY_CONT_DELAYEDINITIALIZATION_DELAYEDINITIALIZATION_HPP
