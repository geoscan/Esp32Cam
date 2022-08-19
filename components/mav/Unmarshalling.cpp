//
// Parser.cpp
//
// Created on: Dec 01, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MAV_DEBUG_LEVEL)
#include <esp_log.h>

#include <algorithm>
#include "utility/Buffer.hpp"
#include "utility/LogSection.hpp"
#include "Unmarshalling.hpp"
#include "mav/mav.hpp"

using namespace Mav;

GS_UTILITY_LOGV_METHOD_SET_ENABLED(Unmarshalling, push, 0)

std::size_t Mav::Unmarshalling::push(Ut::ConstBuffer aBuffer)
{
	auto buffer = aBuffer.as<const std::uint8_t>();
	GS_UTILITY_LOGV_METHOD(Mav::kDebugTag, Unmarshalling, push, "Unmarshalling::push(): processing buffer (%d bytes)",
		buffer.size());
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

	GS_UTILITY_LOGV_METHOD(Mav::kDebugTag, Unmarshalling, push, "Unmarshalling::push(): processed %d bytes", counter);
	return counter;
}
