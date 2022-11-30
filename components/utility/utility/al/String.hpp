//
// String.hpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_STRING_HPP_)
#define UTILITY_UTILITY_STRING_HPP_

#include <cstdint>

namespace Ut {
namespace Al {

bool checkEndswith(const char *aStr, const char *aSuffix);

namespace Str {

using ::Ut::Al::checkEndswith;  // Backward compatibility

/// \brief Constexpr evaluation of string length.
/// \pre `str` MUST be terminated w/ '\0'
constexpr std::size_t cxStrlen(const char *str, std::size_t pos = 0)
{
	return str[pos] == '\0' ? pos : Str::cxStrlen(str, pos + 1);
}

}  // namespace Str
}  // namespace Al
}  // namespace Ut

#endif // UTILITY_UTILITY_STRING_HPP_
