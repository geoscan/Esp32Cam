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
#include "utility/cont/CircularBuffer.hpp"

namespace Ut {

template <typename T>
class Tbuffer;

using ConstBuffer = Tbuffer<const void>;

}  // namespace Ut

namespace Mav {

static constexpr auto kUnmarshallingQueueMaxSize = 1;
using UnmarshallingHoldQueue = typename Ut::Cont::CircularBuffer<mavlink_message_t, kUnmarshallingQueueMaxSize, false>;
using UnmarshallingBaseType = typename std::queue<mavlink_message_t, UnmarshallingHoldQueue>;

class Unmarshalling final : public UnmarshallingBaseType {
public:
	std::size_t push(Ut::Cont::ConstBuffer buffer);

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
