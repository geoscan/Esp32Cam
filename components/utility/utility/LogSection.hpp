//
// LogSection.hpp
//
// Created on: Mar 28, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Enables section-wide debug output.

#ifndef UTILITY_UTILITY_LOGSECTION_HPP_
#define UTILITY_UTILITY_LOGSECTION_HPP_

// RAII wrapper over debug output
#define GS_UTILITY_LOG_SECTION_SUFF_LEVEL(tag, context, cl, instance, command) \
struct cl {\
	inline cl() {command(tag, context " enter"); } \
	inline ~cl() {command(tag, context " exit"); } \
} instance; \
(void) instance

// Implementation detail
#define GS_UTILITY_LOG_DEF_APPEND(x, y) x ## y
#define GS_UTILITY_LOG_SECTION_IMPL(tag, context, suff, command) GS_UTILITY_LOG_SECTION_SUFF_LEVEL(tag, context, GS_UTILITY_LOG_DEF_APPEND(Log,suff), GS_UTILITY_LOG_DEF_APPEND(log,suff), command)

// Debug section - "verbose" level
#define GS_UTILITY_LOG_SECTIONV(tag, context) GS_UTILITY_LOG_SECTION_IMPL(tag,context,__LINE__, ESP_LOGV)

// DEbug section - "debug" level
#define GS_UTILITY_LOG_SECTIOND(tag, context) GS_UTILITY_LOG_SECTION_IMPL(tag,context,__LINE__, ESP_LOGD)

#endif // UTILITY_UTILITY_LOGSECTION_HPP_
