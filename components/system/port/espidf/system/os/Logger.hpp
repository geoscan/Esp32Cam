//
// Logger.hpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_LOGGER_HPP_
#define COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_LOGGER_HPP_

#include "system/os/LogLevel.hpp"
#include <esp_log.h>

namespace Sys {

class Logger {
public:
	template <class ...Ts>
	static inline void write(LogLevel aLogLevel, const char *aTag, const char *aFormat, Ts ...aArgs)
	{
		esp_log_write(logLevelIntoEspLogLevel(aLogLevel), aTag, aFormat, aArgs...);
	}

private:
	static inline esp_log_level_t logLevelIntoEspLogLevel(LogLevel aLogLevel)
	{
		return static_cast<esp_log_level_t>(aLogLevel);
	}
};

}  // Sys

#endif  // COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_LOGGER_HPP_
