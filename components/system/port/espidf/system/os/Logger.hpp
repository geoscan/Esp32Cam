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
#include <mutex>

namespace Sys {

class Logger {
public:
	template <class ...Ts>
	static inline void write(LogLevel aLogLevel, const char *aTag, const char *aFormat, Ts ...aArgs)
	{
		logMutex.lock();
		esp_log_write(logLevelIntoEspLogLevel(aLogLevel), aTag, "%s (%d) %s: ", logLevelToString(aLogLevel),
			esp_log_timestamp(), aTag);
		esp_log_write(logLevelIntoEspLogLevel(aLogLevel), aTag, aFormat, aArgs...);
		esp_log_write(logLevelIntoEspLogLevel(aLogLevel), aTag, "\n");
		logMutex.unlock();
	}

private:
	static inline esp_log_level_t logLevelIntoEspLogLevel(LogLevel aLogLevel)
	{
		return static_cast<esp_log_level_t>(aLogLevel);
	}

	static inline const char *logLevelToString(LogLevel aLogLevel)
	{
		return logLevelToStringMapping[static_cast<std::size_t>(aLogLevel)];
	}

	static const char *logLevelToStringMapping[static_cast<std::size_t>(LogLevel::N)];
	static std::mutex logMutex;
};

}  // Sys

#endif  // COMPONENTS_SYSTEM_PORT_ESPIDF_SYSTEM_OS_LOGGER_HPP_
