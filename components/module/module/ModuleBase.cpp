//
// ModuleBase.cpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "ModuleBase.hpp"
#include "sub/Module.hpp"
#include "utility/thr/WorkQueue.hpp"

namespace Mod {
namespace Fld {

}  // namespace Fld

ModuleBase::ModuleBase(Module aModule) :
	identity{aModule}
{
}

void ModuleBase::notifyFieldAsync(const Fld::ModuleField &moduleField)
{
	if (Ut::Thr::Wq::MediumPriority::checkInstance()) {
		Ut::Thr::Wq::MediumPriority::getInstance().push(
			[moduleField]()
			{
				Sub::Mod::OnModuleFieldSetUpdate::notify(moduleField);
			});
	}
}

void ModuleBase::getFieldValue(Fld::Req aReq, Fld::OnResponseCallback aOnResponse)
{
}

void ModuleBase::setFieldValue(Fld::WriteReq aReq, Fld::OnWriteResponseCallback aCb)
{
}

}  // namespace Mod
