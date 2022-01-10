//
// Parser.hpp
//
// Created on: Dec 01, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef MAV_PRIV_INCLUDE_PARSER_HPP
#define MAV_PRIV_INCLUDE_PARSER_HPP

#include <queue>
#include "Mavlink.hpp"
#include "utility/CircularBuffer.hpp"

namespace Utility {

template <typename T>
class Tbuffer;

using ConstBuffer = Tbuffer<const void>;

}  // namespace Utility

namespace Mav {

static constexpr auto kUnmarshallingQueueMaxSize = 2;
using UnmarshallingHoldQueue = typename Utility::CircularBuffer<mavlink_message_t, kUnmarshallingQueueMaxSize, false>;
using UnmarshallingBaseType = typename std::queue<mavlink_message_t, UnmarshallingHoldQueue>;

class Unmarshalling final : public UnmarshallingBaseType {
public:
	void push(Utility::ConstBuffer buffer);

private:
	using UnmarshallingBaseType::push;
	struct {
		mavlink_message_t rxMessage;
		mavlink_status_t rxStatus;
		mavlink_message_t parsedMessage;
		mavlink_status_t parsedStatus;
	} input;
};

}  // namespace Mav

#endif  // MAV_PRIV_INCLUDE_PARSER_HPP
