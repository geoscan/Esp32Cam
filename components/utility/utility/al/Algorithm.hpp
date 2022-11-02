//
// Algorithm.hpp
//
// Created on: Jun 29, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef UTILITY_UTILITY_ALGORITHM_HPP
#define UTILITY_UTILITY_ALGORITHM_HPP

#include <array>
#include <algorithm>
#include <cassert>
#include <numeric>

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
/// \tparam O Output value type (float or double)
template <typename T, typename O = float>
O normalize(T val, T from, T to)
{
	assert(to - from > std::numeric_limits<float>::epsilon());

	return static_cast<O>(val - from) / static_cast<O>(to - from);
}

}  // namespace Al
}  // namespace Ut

#endif // ALGORITHM_HPP
