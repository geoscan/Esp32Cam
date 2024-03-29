//
// Algorithm.hpp
//
// Created on: Jun 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_ALGORITHM_HPP
#define UTILITY_UTILITY_ALGORITHM_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <type_traits>

namespace Ut {
namespace Al {

/// \brief Checks whether the \arg 'object' is contained in the domain specified by \arg 'values'
template <typename T, typename ...Args>
bool in(const T &object, const Args &...values)
{
	std::array<bool, sizeof...(values)> comparisons{{(object == values)...}};

	return std::any_of(comparisons.begin(), comparisons.end(), [](bool f){return f;});
}

/// \brief Normalizes the value
/// \tparam T Input value type
/// \tparam O Output value type
template <typename T, typename O = float>
O normalize(T val, T from, T to)
{
	assert(to - from > std::numeric_limits<T>::epsilon());

	return static_cast<O>(val - from) / static_cast<O>(to - from);
}

/// \brief De-normalizes the value
/// \tparam T Input value type
/// \tparam O Output value type
template <class T, class O>
O scale(T val, O from, O to)
{
	assert(to - from > std::numeric_limits<T>::epsilon());

	return static_cast<O>((static_cast<T>(to) - static_cast<T>(from)) * val + static_cast<T>(from));
}

/// \brief Fits the value in a specified range
/// \tparam T Input value type
/// \tparam O Output value type
template <typename T, typename O = T>
O clamp(T val, T from, T to)
{
	if (std::is_floating_point<T>::value) {
		assert(to - from > std::numeric_limits<T>::epsilon());
	}

	return static_cast<O>(val) < static_cast<O>(from) ? static_cast<O>(from) :
		static_cast<O>(val) > static_cast<O>(to) ? to :
		val;
}

}  // namespace Al
}  // namespace Ut

#endif // ALGORITHM_HPP
