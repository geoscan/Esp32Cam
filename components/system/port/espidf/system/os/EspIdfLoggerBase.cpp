//
// EspIdfLoggerBase.cpp
//
// Created on: Aug 24, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "system/os/LoggerBase.hpp"
#include <esp_log.h>
#include <array>

namespace Sys {

void LoggerBase::setTagLogLevel(const char *aTag, LogLevel aLogLevel)
{
	static constexpr const std::array<esp_log_level_t, static_cast<std::size_t>(LogLevel::N)> kLogLevelMapping
	esp_log_level_set(aTag, return static_cast<esp_log_level_t>(aLogLevel));
}

}  // Sys
