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

///
/// \brief Statically-allocated return buffer
///
struct Buf {
	std::uint8_t buf[sizeof(mavlink_message_t)];
	std::size_t len;
};

using MarshallingHoldQueue = typename Ut::CircularBuffer<Buf, kMarshallingQueueMaxSize, false>;
using MarshallingBaseType = typename std::queue<mavlink_message_t, MarshallingHoldQueue>;

class Marshalling : public MarshallingBaseType {
private:
	using MarshallingBaseType::push;

public:
	static std::size_t push(const mavlink_message_t &, Ut::Buffer);
	static std::size_t push(const mavlink_message_t &, void *aBuffer);
	std::size_t push(const mavlink_message_t &);
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MARSHALLING_HPP_
