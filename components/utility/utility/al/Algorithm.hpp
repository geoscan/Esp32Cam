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

namespace Ut {

namespace Algorithm {

/// \brief Checks whether the \arg 'object' is contained in the domain specified by \arg 'values'
template <typename T, typename ...Args>
bool in(const T &object, const Args &...values)
{
	std::array<bool, sizeof...(values)> comparisons{{(object == values)...}};

	return std::any_of(comparisons.begin(), comparisons.end(), [](bool f){return f;});
}

}  // namespace Algorithm

}  // namespace Ut

#endif // ALGORITHM_HPP
