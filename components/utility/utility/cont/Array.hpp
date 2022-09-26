//
// Array.hpp
//
// Created on: Sep 23, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_CONT_ARRAY_HPP_)
#define UTILITY_UTILITY_CONT_ARRAY_HPP_

#include <array>

namespace Ut {
namespace Cont {

template <class T, class ...Ts>
inline auto makeArray(Ts &&...aArgs) -> std::array<T, sizeof...(Ts)>
{
	return std::array<T, sizeof...(Ts)>{{std::forward<Ts>(aArgs)...}};
}

}  // namespace Cont
}  // namespace Ut

#endif // UTILITY_UTILITY_CONT_ARRAY_HPP_
