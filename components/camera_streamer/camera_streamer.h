//
// camera_streamer.h
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_CAMERA_STREAMER_H
#define COMPONENTS_CAMERA_STREAMER_H

#include "sdkconfig.h"
#include <asio.hpp>

void cameraStreamerStart(asio::io_context &);

#endif // COMPONENTS_CAMERA_STREAMER_H
