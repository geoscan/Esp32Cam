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
	return mavlink_msg_to_send_buffer(static_cast<std::uint8_t *>(aBuffer.data()), &aMavlinkMessage);
}

std::size_t Mav::Marshalling::push(const mavlink_message_t &aMavlinkMessage, void *aBuffer)
{
	return mavlink_msg_to_send_buffer(static_cast<uint8_t *>(aBuffer), &aMavlinkMessage);
}

std::size_t Mav::Marshalling::push(const mavlink_message_t &aMessage)
{
	auto ret = 0;

	if (size() < kMarshallingQueueMaxSize) {
		BaseType::push({});
		ret = push(aMessage, {&back(), sizeof(mavlink_message_t)});
	}

	return ret;
}
