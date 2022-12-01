//
// ModuleBase.cpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "ModuleBase.hpp"
#include "sub/Module.hpp"
#include "module/Parameter/ParameterDescription.hpp"
#include "module/Parameter/Parameter.hpp"
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
				Sub::Mod::OnModuleFieldUpdate::notify(moduleField);
			});
	}
}

void ModuleBase::getFieldValue(Fld::Req aReq, Fld::OnResponseCallback aOnResponse)
{
}

void ModuleBase::setFieldValue(Fld::WriteReq aReq, Fld::OnWriteResponseCallback aCb)
{
}

void ModuleBase::fieldTryMirrorParameter(Module module, Fld::Field field, const Variant &variant)
{
	const Par::ParameterDescription *parameterDescription = Par::Parameter::descriptionByMf(module, field);

	if (parameterDescription != nullptr && parameterDescription->mirrorField) {
		Par::Parameter *parameter = Par::Parameter::instanceByMf(module, field);

		if (parameter != nullptr) {
			parameter->set(variant);
			parameter->commit();
		}
	}
}

}  // namespace Mod
