//
// Ap.hpp
//
// Created on: Dec 01, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(WIFI_WIFI_AP_HPP_)
#define WIFI_WIFI_AP_HPP_

#include "module/ModuleBase.hpp"

namespace Wifi {

/// \brief Module object. Encapsulates Wi-Fi Access Point-related field
/// processing
class Ap : public Mod::ModuleBase {
public:
	/// \brief Serves as a stub interpcepting field setting. For now, always
	/// returns OK, to trigger parameters mirroring.
	void setFieldValue(Mod::Fld::WriteReq aReq, Mod::Fld::OnWriteResponseCallback aCb) override;
};

}  // namespace Wifi

#endif // WIFI_WIFI_AP_HPP_
