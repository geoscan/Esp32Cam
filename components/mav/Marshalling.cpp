//
// Marshalling.cpp
//
// Created on: Dec 02, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <cassert>
#include "Mavlink.hpp"
#include "utility/Buffer.hpp"
#include "Marshalling.hpp"

std::size_t Mav::Marshalling::push(const mavlink_message_t &aMavlinkMessage, Utility::Buffer aBuffer)
{
	assert(aBuffer.size() >= sizeof(mavlink_message_t));
	return mavlink_msg_to_send_buffer(static_cast<std::uint8_t *>(aBuffer.data()), &aMavlinkMessage);
}

void Mav::Marshalling::push(const mavlink_message_t &aMessage)
{
	if (size() < kMarshallingQueueMaxSize) {
		BaseType::push({});
		push(aMessage, {&back(), sizeof(mavlink_message_t)});
	}
}
