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

// Method-level logging. Enables one to enable or disable logging for a particular method. Useful for debugging purposes

template <class R, class T, class ...Ts>
struct GsUtilityLogMember {
	template <R (T::* M)(Ts...)>
	struct Impl {
	};
};

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (T::*)(Ts...)) -> GsUtilityLogMember<R, T, Ts...>;

template <class R, class T, class ...Ts>
struct GsUtilityLogMemberConst {
	template <R (T::* M)(Ts...) const>
	struct Impl {
	};
};

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (T::*)(Ts...) const) -> GsUtilityLogMemberConst<R, T, Ts...>;

template <class R, class T, class ...Ts>
struct GsUtilityLogMemberVolatile {
	template <R (T::* M)(Ts...) volatile>
	struct Impl {
	};
};

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (T::*)(Ts...) volatile) -> GsUtilityLogMemberVolatile<R, T, Ts...>;

template <class R, class T, class ...Ts>
struct GsUtilityLogMemberCv {
	template <R (T::* M)(Ts...) const volatile>
	struct Impl {
	};
};

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (T::*)(Ts...) const volatile) -> GsUtilityLogMemberCv<R, T, Ts...>;

template <class R, class T, class ...Ts>
struct GsUtilityLogFunction {
	template <R (*F)(Ts...)>
	struct Impl {
	};
};

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (*)(Ts...)) -> GsUtilityLogFunction<R, T, Ts...>;

template <class T>
struct GsUtilityLogMethodV {
	static constexpr bool enabled = false;
};

/// \brief False-fallback, default implementation for debug level.
///
template <class T>
struct GsUtilityLogMethodD {
	static constexpr bool enabled = false;
};

template <class T, unsigned A>
struct GsUtilityLogClassAspect;

#define GS_UTILITY_DEBUG_LEVEL_ENABLED (LOG_LOCAL_LEVEL >= 4)
#define GS_UTILITY_VERBOSE_LEVEL_ENABLED (LOG_LOCAL_LEVEL >= 5)

// Definitions generating compile-time boolean logging flags for particular method (GsUtilityLogMethod(V|D))

#define GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) \
	typename decltype(gsUtilityRevealType< cls >(&cls::method))::Impl<&cls::method>

#define GS_UTILITY_LOG_CLASS_ASPECT_MARKER_TYPE(cls, aspectid) \
	GsUtilityLogClassAspect < cls, static_cast<unsigned>( aspectid ) >

#define GS_UTILITY_LOG_METHOD_STRUCT_DEFINE_IMPL(structname, marker, en) \
template<> \
struct structname < marker > { \
	static constexpr bool enabled = en; \
};

#define GS_UTILITY_LOG_METHOD_STRUCT_CALL_IMPL(structname, esplogdefine, markertype, ...) \
do { \
	if (structname < markertype >::enabled) { \
		esplogdefine (__VA_ARGS__ ); \
	} \
} while (0)

// Wrappers over struct generators

#define GS_UTILITY_LOG_METHOD_STRUCT_DEFINE(level, cls, method, en) \
	GS_UTILITY_LOG_METHOD_STRUCT_DEFINE_IMPL(GS_UTILITY_LOG_DEF_APPEND(GsUtilityLogMethod, level), \
	GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method), en)

#define GS_UTILITY_LOG_METHOD_STRUCT_CALL(level, tag, cls, method, ...) \
	GS_UTILITY_LOG_METHOD_STRUCT_CALL_IMPL(GS_UTILITY_LOG_DEF_APPEND(GsUtilityLogMethod, level), \
	GS_UTILITY_LOG_DEF_APPEND(ESP_LOG, level), GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method), \
	tag, #cls "::" #method "() " __VA_ARGS__)

#define GS_UTILITY_LOG_CLASS_ASPECT_DEFINE(level, cls, aspectid, en) \
	GS_UTILITY_LOG_METHOD_STRUCT_DEFINE_IMPL(GS_UTILITY_LOG_DEF_APPEND(GsUtilityLogMethod, level), \
	GS_UTILITY_LOG_CLASS_ASPECT_MARKER_TYPE(cls, aspectid), en)

#define GS_UTILITY_LOG_CLASS_ASPECT_CALL(level, tag, cls, aspect, ...) \
	GS_UTILITY_LOG_METHOD_STRUCT_CALL_IMPL(GS_UTILITY_LOG_DEF_APPEND(GsUtilityLogMethod, level), \
	GS_UTILITY_LOG_DEF_APPEND(ESP_LOG, level), GS_UTILITY_LOG_CLASS_ASPECT_MARKER_TYPE(cls, aspect), \
	tag, #cls "(" #aspect ") " __VA_ARGS__)

// User-level defines accessing struct generators

/// \defgroup GS_UTILITY_LOG_METHOD \brief Method-level logging macros akin to topics
///
/// \details "METHOD_SET_ENABLED" macros use template specialization mechanism to define `enabled` flags for individual
/// methods. It is not necessary to call those, because there is a default false-fallback for undefined (CLASS, METHOD)
/// pairs (see GsUtilityLogMethodD for details)
///
/// Their counterparts, "METHOD" macros invoke standard ESP-IDF's logging functions according to the logging level.
///
/// @{
///

#define GS_UTILITY_LOGV_METHOD_SECTION(tag, cls, method, commentstr) \
	GS_UTILITY_LOG_METHOD_STRUCT_CALL_IMPL(\
	GsUtilityLogMethodV, \
	GS_UTILITY_LOG_SECTIONV, \
	GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method), \
	tag, #cls "::" #method "() " commentstr)

#define GS_UTILITY_LOGV_METHOD_SET_ENABLED(cls, method, en) \
	GS_UTILITY_LOG_METHOD_STRUCT_DEFINE(V, cls, method, (en && GS_UTILITY_VERBOSE_LEVEL_ENABLED))

#define GS_UTILITY_LOGV_METHOD(tag, cls, method, ...) \
	GS_UTILITY_LOG_METHOD_STRUCT_CALL(V, tag, cls, method, __VA_ARGS__)

#define GS_UTILITY_LOGD_METHOD_SET_ENABLED(cls, method, en) \
	GS_UTILITY_LOG_METHOD_STRUCT_DEFINE(D, cls, method, (en && GS_UTILITY_DEBUG_LEVEL_ENABLED))

#define GS_UTILITY_LOGD_METHOD(tag, cls, method, ...) \
	GS_UTILITY_LOG_METHOD_STRUCT_CALL(D, tag, cls, method, __VA_ARGS__)

#define GS_UTILITY_LOGV_CLASS_ASPECT_SET_ENABLED(cls, aspect, en) \
	GS_UTILITY_LOG_CLASS_ASPECT_DEFINE(V, cls, aspect, en)

#define GS_UTILITY_LOGV_CLASS_ASPECT(tag, cls, aspect, ...) \
	GS_UTILITY_LOG_CLASS_ASPECT_CALL(V, tag, cls, aspect, __VA_ARGS__)

#define GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(cls, aspect, en) \
	GS_UTILITY_LOG_CLASS_ASPECT_DEFINE(D, cls, aspect, en)

#define GS_UTILITY_LOGD_CLASS_ASPECT(tag, cls, aspect, ...) \
	GS_UTILITY_LOG_CLASS_ASPECT_CALL(D, tag, cls, aspect, __VA_ARGS__)

#define GS_UTILITY_LOG_EVERY_N_TURNS(nturns, ...) \
{ \
	static unsigned turn = 0; \
	if ((turn++) % nturns == 0) { \
		__VA_ARGS__ ; \
	} \
}

/// @}

#endif // UTILITY_UTILITY_LOGSECTION_HPP_
