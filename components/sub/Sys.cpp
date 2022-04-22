//
// Sys.cpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "sub/Sys.hpp"

namespace Sub {
namespace Sys {
namespace Fld {

bool Req::shouldRespond(Module aThisModule)
{
	return Utility::Algorithm::in(aThisModule, module, Module::All);
}

}  // namespace Fld
}  // namespace Sys
}  // namespace Sub
