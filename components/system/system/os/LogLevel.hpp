//
// LogLevel.hpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_SYSTEM_SYSTEM_OS_LOGLEVEL_HPP
#define COMPONENTS_SYSTEM_SYSTEM_OS_LOGLEVEL_HPP

namespace Sys {

enum class LogLevel {
	Error = 1,
	Warning,
	Info,
	Debug,
	Verbose,
};

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_OS_LOGLEVEL_HPP
