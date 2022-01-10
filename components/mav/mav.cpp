//
// mav.cpp
//
// Created on: Dec 28, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "mav/mav.hpp"
#include "Mavlink.hpp"
#include "Marshalling.hpp"
#include "Unmarshalling.hpp"
#include "Microservice/GsNetwork.hpp"
#include "Dispatcher.hpp"

namespace Mav {

void init()
{
	static Dispatcher dispatcher;
	(void)dispatcher;
}

}  // namespace Mav
