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

std::size_t Mav::Unmarshalling::push(Utility::ConstBuffer aBuffer)
{
	auto buffer = aBuffer.as<const std::uint8_t>();
	std::size_t counter = 0;

	for (auto *ch = buffer.data(); ch < buffer.data() + buffer.size() && size() < kUnmarshallingQueueMaxSize;
		++ch, ++counter)
	{
		if (mavlink_frame_char_buffer(&input.rxMessage, &input.rxStatus, *ch, &input.parsedMessage,
			&input.parsedStatus) == MAVLINK_FRAMING_OK)
		{
			UnmarshallingBaseType::push(input.parsedMessage);
		}
	}

	return counter;
}
