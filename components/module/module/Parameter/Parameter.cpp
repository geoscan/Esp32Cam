//
// Api.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "module/Parameter/ParameterDescription.hpp"
#include "utility/al/String.hpp"
#include <array>
#include <algorithm>
#include "Parameter.hpp"

namespace Mod {
namespace Par {

static constexpr std::array<ParameterDescription, 1> kParameterDescriptions{{
	{
		0,  // id
		"WifiApSsid",  // name
		ParameterType::Str,
		Mod::Module::WifiAp,
		Mod::Fld::Field::StringIdentifier,
	}
}};

/// \brief Max length of a parameter's string name. It is kept being equal 16, for compatibility w/ MAVLink
constexpr std::size_t kParameterStrlen = 16;

/// \brief Compile-time validation of parameters' name length
constexpr bool pdescValidateStrlen(std::size_t pos = 0)
{
	return pos >= kParameterDescriptions.size() ?
		true :
		Ut::Al::Str::cxStrlen(kParameterDescriptions[pos].name) <= kParameterStrlen && pdescValidateStrlen(pos + 1);
}

static_assert(pdescValidateStrlen(), "A parameter's name length has been exceeded");

bool Parameter::toId(Module module, Fld::Field field, std::size_t &oId)
{
	bool res = false;
	auto it = std::find_if(std::begin(kParameterDescriptions), std::end(kParameterDescriptions),
		[module, field](const ParameterDescription &aDescription)
		{
			return module == aDescription.module && field == aDescription.field;
		});

	if (it != std::end(kParameterDescriptions)) {
		res = true;
		oId = it->id;
	}

	return res;
}


}  // namespace Par
}  // namespace Mod
