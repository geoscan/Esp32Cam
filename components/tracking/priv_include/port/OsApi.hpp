//
// OsApi.hpp
//
// Created on: Oct 04, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(TRACKING_PRIV_INCLUDE_PORT_OSAPI_HPP_)
#define TRACKING_PRIV_INCLUDE_PORT_OSAPI_HPP_

#include <embmosse/Port/OsApi.hpp>

namespace Trk {

class OsApi : Mosse::Port::OsApi {
public:
	OsApi();
	void taskYieldMinDelay() override;
};

}  // namespace Trk

#endif // TRACKING_PRIV_INCLUDE_PORT_OSAPI_HPP_
