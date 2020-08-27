//
// rtsp.h
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef RTSP_H
#define RTSP_H

#include "sdkconfig.h"

constexpr unsigned kRtpPort = CONFIG_RTP_UDP_PORT;
constexpr unsigned kRtspPort = CONFIG_RTSP_TCP_PORT;

void rtspStart();

#endif // RTSP_H
