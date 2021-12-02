//
// Marshalling.hpp
//
// Created on: Dec 02, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_MARSHALLING_HPP_
#define MAV_PRIV_INCLUDE_MARSHALLING_HPP_

#include <queue>
#include <vector>
#include <cstdint>
#include <type_traits>
#include "Mavlink.hpp"

namespace Utility {

template <typename T>
class Tbuffer;

using Buffer = Tbuffer<void>;
using ConstBuffer = Tbuffer<const void>;

}  // namespace Utility

struct __mavlink_message;
typedef struct __mavlink_message mavlink_message_t;

namespace Mav {

struct MarshallingBase {
public:
	static void push(const mavlink_message_t &, Utility::Buffer);

protected:
	static constexpr std::size_t kMavlinkMessageMaxLength = 280;
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MARSHALLING_HPP_
