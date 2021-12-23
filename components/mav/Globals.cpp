//
// Globals.cpp
//
// Created on: Dec 23, 2021
//     Author: Dmitry Murashov (dmtrDOTmurashovATgmailDOTcom (gmail.com))
//

#include "Globals.hpp"
#include "Mavlink.hpp"

constexpr unsigned char Mav::Globals::getSysId()
{
	return 1;
}

constexpr unsigned char Mav::Globals::getCompId()
{
	return MAV_COMP_ID_UDP_BRIDGE;
}
