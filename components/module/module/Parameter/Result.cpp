//
// Result.cpp
//
// Created on: Nov 29, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Result.hpp"
#include <array>
#include <cassert>

namespace Mod {
namespace Par {

constexpr std::array<const char *, static_cast<std::size_t>(Result::N)> kResultMapping {{
	"Success",
	"ERROR: Memory provider not found",
	"ERROR: SD card",
	"ERROR: File I/O",
	"ERROR: Config file format violation",
	"ERROR: Not enough memory",
	"ERROR: Could not find config entry",
	"ERROR: Config does not exist"
}};

const char *resultAsStr(Result result)
{
	assert(static_cast<std::size_t>(result) < static_cast<std::size_t>(Result::N));
	return kResultMapping[static_cast<std::size_t>(result)];
}

}  // namespace Par
}  // namespace Mod
