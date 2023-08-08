//
// Sub.hpp
//
// Created on: Aug 08, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//


#ifndef WIFI_WIFI_SUB_HPP
#define WIFI_WIFI_SUB_HPP

#include "sub/Subscription.hpp"
#include <asio.hpp>

namespace Wifi {
namespace Sub {

/// Static marker
struct WifiDisconnectedTopic;

/// "Wi-Fi disconnected" event key
using Disconnected = typename ::Sub::IndKey<void(asio::ip::address), WifiDisconnectedTopic>;

}  // Sub
}  // Wifi

#endif // WIFI_WIFI_SUB_HPP
