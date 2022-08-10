//
// Sta.hpp
//
// Created on: Aug 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "module/ModuleBase.hpp"
#include <esp_netif.h>

namespace Wifi {

class Sta : public Mod::ModuleBase {
public:
	Sta(esp_netif_t **);
	void getFieldValue(Mod::Fld::Req aReq, Mod::Fld::OnResponseCallback aOnResponse);

private:
	esp_netif_t **espNetif;
};

}  // namespace Wifi
