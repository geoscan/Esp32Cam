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

struct __mavlink_message;
typedef __mavlink_message mavlink_message_t;

namespace Mav {

using HoldQueue = std::queue<Utility::Thold<std::uint8_t, sizeof(mavlink_message_t)>>;
class Marshalling : public HoldQueue {
private:
	using HoldQueue::push;

public:
	static std::size_t push(const mavlink_message_t &, Utility::Buffer);

	///
	/// \brief push
	/// \return true, if there was room for one message
	///
	void push(const mavlink_message_t &);
};

}  // namespace Mav

#endif // MAV_PRIV_INCLUDE_MARSHALLING_HPP_
