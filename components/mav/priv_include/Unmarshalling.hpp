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

namespace Utility {

template <typename T>
class Tbuffer;

using ConstBuffer = Tbuffer<const void>;

}  // namespace Utility

namespace Mav {

class Unmarshalling final : public std::queue<mavlink_message_t> {
public:
	using std::queue<mavlink_message_t>::queue;

	void push(Utility::ConstBuffer buffer);

private:
	using std::queue<mavlink_message_t>::push;
private:
	struct {
		mavlink_message_t rxMessage;
		mavlink_status_t rxStatus;
		mavlink_message_t parsedMessage;
		mavlink_status_t parsedStatus;
	} input;
};

}  // namespace Mav

#endif  // MAV_PRIV_INCLUDE_PARSER_HPP
