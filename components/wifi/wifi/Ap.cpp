//
// Ap.cpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "Ap.hpp"

namespace Wifi {

Ap::Ap() : ModuleBase{Mod::Module::WifiAp}
{
}

void Ap::setFieldValue(Mod::Fld::WriteReq, Mod::Fld::OnWriteResponseCallback onWriteResponse)
{
	onWriteResponse({Mod::Fld::RequestResult::Ok});
}

}  // namespace Wifi
