//
// LogSection.hpp
//
// Created on: Mar 28, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

// Enables section-wide debug output.

/// \file

#ifndef UTILITY_UTILITY_LOGSECTION_HPP_
#define UTILITY_UTILITY_LOGSECTION_HPP_

#if !defined(__ESP_LOG_H__)
# error "<esp_log.h> must be included before LogSection.hpp"
#endif

#if !defined(LOG_LOCAL_LEVEL)
# error "LogSection.hpp relies on presence of LOG_LEVEL_LOCAL macro which can be included from <esp_log.h>"
#endif

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

// Debug section - "debug" level
#define GS_UTILITY_LOG_SECTIOND(tag, context) GS_UTILITY_LOG_SECTION_IMPL(tag,context,__LINE__, ESP_LOGD)

template <class R, class T, class ...Ts>
struct GsUtilityLogMember {
	template <R (T::* M)(Ts...)>
	struct Impl {
	};
};

template <class R, class T, class ...Ts>
struct GsUtilityLogFunction {
	template <R (*F)(Ts...)>
	struct Impl {
	};
};

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (T::*)(Ts...)) -> GsUtilityLogMember<R, T, Ts...>;

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (*)(Ts...)) -> GsUtilityLogFunction<R, T, Ts...>;

#define GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) \
	typename decltype(gsUtilityRevealType< cls >(&cls::method))::Impl<&cls::method>

template <class T>
struct GsUtilityLogvMethod {
	static constexpr bool enabled = false;
};

template <class T>
struct GsUtilityLogdMethod {
	static constexpr bool enabled = false;
};

#define GS_UTILITY_DEBUG_LEVEL_ENABLED (LOG_LOCAL_LEVEL >= 4)
#define GS_UTILITY_VERBOSE_LEVEL_ENABLED (LOG_LOCAL_LEVEL >= 5)

/// \defgroup GS_UTILITY_ Log-related macros
/// @{

/// \brief Uses template specialization mechanism to define `enabled` flags for individual methods.
///
/// \details The default implementation provides false-fallback, so using `GS_UTILITY_LOGV_METHOD_SET_ENABLED` for
/// disabling logging for a particular method not necessary
///
#define GS_UTILITY_LOGV_METHOD_SET_ENABLED(cls, method, en) \
template<> \
struct GsUtilityLogvMethod< GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) > { \
	static constexpr bool enabled = en; \
};

/// \brief Based on `enabled` flag (see `GS_UTILITY_LOGV_METHOD_SET_ENABLED`), either ignores this or invokes
/// `ESP_LOGV` macro.
///
#define GS_UTILITY_LOGV_METHOD(tag, cls, method, ...) \
struct cls##method ; \
do { \
	if (GsUtilityLogvMethod< GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) >::enabled) { \
		ESP_LOGV(tag, #cls "::" #method "() " __VA_ARGS__ ); \
	} \
} while (0)

/// \brief Same as `GS_UTILITY_LOGV_METHOD_SET_ENABLED`, but for "debug" log level
///
#define GS_UTILITY_LOGD_METHOD_SET_ENABLED(cls, method, en) \
template<> \
struct GsUtilityLogdMethod< GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) > { \
	static constexpr bool enabled = en; \
};

/// \brief Same as `GS_UTILITY_LOGV_METHOD`, but for "debug" log level
#define GS_UTILITY_LOGD_METHOD(tag, cls, method, ...) \
struct cls##method ; \
do { \
	if (GsUtilityLogdMethod< GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) >::enabled) { \
		ESP_LOGD(tag, #cls "::" #method "() " __VA_ARGS__ ); \
	} \
} while (0)

/// @}

#endif // UTILITY_UTILITY_LOGSECTION_HPP_
