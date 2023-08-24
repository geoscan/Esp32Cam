//
// Logger.cpp
//
// Created on: Aug 24, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "Logger.hpp"

namespace Sys {

const char *Logger::logLevelToStringMapping[static_cast<std::size_t>(LogLevel::N)] = {
	"",
	"E",
	"W",
	"I",
	"D",
	"V",
};

std::mutex Logger::logMutex{};

}  // Sys
