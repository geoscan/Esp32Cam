//
// Marshalling.hpp
//
// Created on: Dec 02, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_MARSHALLING_HPP_
#define MAV_PRIV_INCLUDE_MARSHALLING_HPP_

#include <queue>
#include "utility/Buffer.hpp"
#include "utility/CircularBuffer.hpp"

struct __mavlink_message;
typedef __mavlink_message mavlink_message_t;

namespace Mav {

static constexpr unsigned kMarshallingQueueMaxSize = 1;
using MarshallingHoldQueue = typename Utility::CircularBuffer<mavlink_message_t, kMarshallingQueueMaxSize, false>;
using MarshallingBaseType = typename std::queue<mavlink_message_t, MarshallingHoldQueue>;

class Marshalling : public std::queue<MarshallingHoldQueue> {
private:
	using BaseType = typename std::queue<MarshallingHoldQueue>;
	using BaseType::push;

public:
	static std::size_t push(const mavlink_message_t &, Utility::Buffer);
	std::size_t push(const mavlink_message_t &);
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MARSHALLING_HPP_
