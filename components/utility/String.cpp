//
// String.cpp
//
// Created on: May 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/String.hpp"
#include <algorithm>

namespace Utility {
namespace Str {

bool checkEndswith(const char *aStr, const char *aSuffix)
{
	const auto *aStrLen = std::strlen(aStr);
	const auto *aSuffixLen = std::strlen(aStr);
	bool ret = false;

	if (aStrLen > 0 && aSuffixLen > 0 && aStrLen >= aSuffixLen) {
		auto *pos = std::find_end(aStr, aStr + aStrLen, aSuffix, aSuffix + aSuffixLen);

		if (aStr + aStrLen != pos) {
			ret = true;
		}
	}

	return ret;
}

}  // namespace Str
}  // namespace Utility
