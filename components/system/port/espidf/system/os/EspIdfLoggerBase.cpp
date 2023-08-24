//
// EspIdfLoggerBase.cpp
//
// Created on: Aug 24, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "system/os/LoggerBase.hpp"
#include <esp_log.h>
#include <array>
#include <cstdint>

namespace Sys {

void LoggerBase::setTagLogLevel(const char *aTag, LogLevel aLogLevel)
{
	static constexpr const std::array<esp_log_level_t, static_cast<std::size_t>(LogLevel::N)> kLogLevelMapping {{
		ESP_LOG_NONE,
		ESP_LOG_ERROR,
		ESP_LOG_WARN,
		ESP_LOG_DEBUG,
		ESP_LOG_VERBOSE,
	}};
	esp_log_level_set(aTag, kLogLevelMapping[static_cast<std::size_t>(aLogLevel)]);
}

}  // Sys
