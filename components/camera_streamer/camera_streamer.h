#ifndef COMPONENTS_CAMERA_STREAMER_H
#define COMPONENTS_CAMERA_STREAMER_H

#include "sdkconfig.h"

constexpr const unsigned kSourcePort = CONFIG_CAMSTREAM_SOURCE_UDP_PORT;
constexpr const unsigned kSinkPort   = CONFIG_CAMSTREAM_SINK_UDP_PORT;

#if CONFIG_CAMSTREAM_USE_FPS
constexpr const short kFps = CONFIG_CAMSTREAM_FPS;
#else
constexpr const short kFps = -1;
#endif // CONFIG_CAMSTREAM_USE_FPS


#endif // COMPONENTS_CAMERA_STREAMER_H
