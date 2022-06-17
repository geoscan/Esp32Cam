//
// Sys.cpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/mod/ModuleBase.hpp"

namespace Sub {
namespace Sys {
namespace Fld {

bool Req::shouldRespond(Module aThisModule)
{
	return Utility::Algorithm::in(module, aThisModule, Module::All);
}

}  // namespace Fld

ModuleBase::ModuleBase(ModuleType aModuleType) :
	identity{aModuleType},
	key{{&ModuleBase::getFieldValueIpc, this}}
{
}

typename Fld::ModuleGetFieldMult::Ret ModuleBase::getFieldValue(typename Fld::ModuleGetFieldMult::Arg<0>,
	typename Fld::ModuleGetFieldMult::Arg<1>)
{
}

/// \brief Wraps `getFieldValue`
///
/// \param aReq - request info
/// \param aCb  - callback receiving a result. A callback is used for the case of multiple, or delayed returns
///               (multiple fields)
///
/// A wrapper is used, because there is no guarantee of portability for the underlying IPC mechanism which would
/// reinterpret-cast a pointer to a virtual method of `ModuleBase` class to that of type `Rr::Object`.
///
typename Fld::ModuleGetFieldMult::Ret ModuleBase::getFieldValueIpc(typename Fld::ModuleGetFieldMult::Arg<0> aReq,
	typename Fld::ModuleGetFieldMult::Arg<1> aCb)
{
	if (aReq.shouldRespond(identity.type)) {
		getFieldValue(aReq, aCb);
	}
}

}  // namespace Sys
}  // namespace Sub
