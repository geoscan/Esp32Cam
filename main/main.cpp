//
// main.cpp
//
// Created on: Aug 12, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "wifi.h"
#include "http.h"

extern "C" int app_main(void)
{
	wifiStart();
	httpStart();
	return 0;
}
