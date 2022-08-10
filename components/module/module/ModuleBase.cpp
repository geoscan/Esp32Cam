//
// ModuleBase.cpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "ModuleBase.hpp"

namespace Mod {
namespace Fld {

}  // namespace Fld

ModuleBase::ModuleBase(Module aModule) :
	identity{aModule}
{
}

void ModuleBase::getFieldValue(Fld::Req aReq, Fld::OnResponseCallback aOnResponse)
{
}

void ModuleBase::setFieldValue(Fld::WriteReq aReq, Fld::OnWriteResponseCallback aCb)
{
}

}  // namespace Mod
