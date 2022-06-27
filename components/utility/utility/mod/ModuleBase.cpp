//
// Sys.cpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/mod/ModuleBase.hpp"

namespace Utility {
namespace Mod {
namespace Fld {

bool Req::shouldRespond(Module aThisModule)
{
	return Utility::Algorithm::in(module, aThisModule, Module::All);
}

}  // namespace Fld

ModuleBase::ModuleBase(ModuleType aModuleType) :
	identity{aModuleType}
{
}

ModuleType ModuleBase::getModuleType() const
{
	return identity.type;
}

void ModuleBase::getFieldValue(Fld::Req aReq, Fld::OnResponseCallback aOnResponse)
{
}

}  // namespace Mod
}  // namespace Utility
