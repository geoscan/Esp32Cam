//
// String.cpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/String.hpp"
#include <algorithm>
#include <cstring>

namespace Utility {
namespace Str {

bool checkEndswith(const char *aStr, const char *aSuffix)
{
	const auto aStrLen = std::strlen(aStr);
	const auto aSuffixLen = std::strlen(aSuffix);
	bool ret = false;

	if (aStrLen > 0 && aSuffixLen > 0 && aStrLen >= aSuffixLen) {
		auto *pos = std::find_end(aStr, aStr + aStrLen, aSuffix, aSuffix + aSuffixLen);

		if (aStr + aStrLen - aSuffixLen == pos) {
			ret = true;
		}
	}

	return ret;
}

}  // namespace Str
}  // namespace Utility
