//
// camera_streamer.h
//
// Created on:  Sep 14, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef COMPONENTS_CAMERA_STREAMER_H
#define COMPONENTS_CAMERA_STREAMER_H

#include "sdkconfig.h"

constexpr const unsigned kSourceUdpPort = CONFIG_CAMSTREAM_SOURCE_UDP_PORT;
constexpr const unsigned kSourceTcpPort = CONFIG_CAMSTREAM_SOURCE_TCP_PORT;

void cameraStreamerStart();

#endif // COMPONENTS_CAMERA_STREAMER_H
