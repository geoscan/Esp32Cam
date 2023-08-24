//
// LoggerBase.hpp
//
// Created on: Aug 24, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//


#ifndef COMPONENTS_SYSTEM_SYSTEM_OS_LOGGERBASE_HPP
#define COMPONENTS_SYSTEM_SYSTEM_OS_LOGGERBASE_HPP

#include "system/os/LogLevel.hpp"

namespace Sys {

class LoggerBase {
public:
	/// Application-wide log level, will be used for each tag unless
	/// `setTagLogLevel` hasn't been called
	static inline void setApplicationLogLevel(LogLevel aLogLevel)
	{
		applicationLogLevel = aLogLevel;
	}

	/// Sets debug level for a specific tag.
	///
	/// \note For the time being, this function is implemented in `port`, as
	/// ESP IDF provides an efficient solution based on hashing. Such an
	/// approach spares time expenses. However, further implementation should
	/// be platform-independent.
	static void setTagLogLevel(const char *aTag, LogLevel aTagLogLevel);

protected:
	static inline LogLevel getApplicationLogLevel()
	{
		return applicationLogLevel;
	}

private:
	static LogLevel applicationLogLevel;
};

}  // Sys

#endif  // COMPONENTS_SYSTEM_SYSTEM_OS_LOGGERBASE_HPP
