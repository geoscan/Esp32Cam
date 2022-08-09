//
// Sta.hpp
//
// Created on: Aug 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "utility/mod/ModuleBase.hpp"

namespace Wifi {

class Sta : public Utility::Mod::ModuleBase {
public:
	Sta();
	void getFieldValue(Utility::Mod::Fld::Req aReq, Utility::Mod::Fld::OnResponseCallback aOnResponse);
};

}  // namespace Wifi
