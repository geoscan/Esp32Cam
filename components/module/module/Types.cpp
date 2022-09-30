//
// Types.cpp
//
// Created on: Aug 11, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Types.hpp"

namespace Mod {
namespace Fld {

const char *RequestResult::toCstr(Mod::Fld::RequestResult::Result aResult)
{
	static constexpr std::array<const char *, static_cast<unsigned>(Result::N)> names {{
		"Ok",
		"Storage error",
		"Value is out of range, or unacceptible",
		"",
	}};
	return names[static_cast<unsigned>(aResult)];
}

}  // namespace Fld
}  // namespace Mod
