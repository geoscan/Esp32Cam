//
// Parser.cpp
//
// Created on: Dec 01, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <algorithm>
#include "utility/Buffer.hpp"
#include "Unmarshalling.hpp"

using namespace Mav;

void Mav::Unmarshalling::push(Utility::ConstBuffer aBuffer)
{
	auto buffer = aBuffer.as<const std::uint8_t>();

	std::for_each(buffer.data(), buffer.data() + buffer.size(), [this](uint8_t ch) {
		if (mavlink_frame_char_buffer(&input.rxMessage, &input.rxStatus, ch, &input.parsedMessage, &input.parsedStatus) == MAVLINK_FRAMING_OK) {
			std::queue<mavlink_message_t>::push(input.parsedMessage);
		}
	});
}
