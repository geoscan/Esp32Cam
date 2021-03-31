//
// Record.cpp
//
// Created on: Mar 30, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "camera_recorder/Record.hpp"

using namespace CameraRecorder;

Record::Record() : key(&Record::onNewFrame, this, false)
{
}

Record::~Record()
{
	key.enableSubscribe(false);
}