//
// Api.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "module/Parameter/ParameterDescription.hpp"
#include <array>
#include "Api.hpp"

namespace Mod {
namespace Par {

static constexpr std::array<ParameterDescription, 1> kParameterDescriptions{{
	{
		"WifiApSsid",  // name
		ParameterType::Str,
		0,  // id
		Mod::Module::WifiAp,
		Mod::Fld::Field::StringIdentifier,
	}
}};

Api::Api() : Ut::MakeSingleton<Api>{*this}
{
}

}  // namespace Par
}  // namespace Mod
