//
// rtsp.cpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "asio.hpp"
#include "RtspServer.hpp"

void rtspStart()
{
    asio::io_context context;
    RtpServer rtpServer(context, 1234);
    RtspServer rtspServer(context, 554, rtpServer);
    context.run();
}