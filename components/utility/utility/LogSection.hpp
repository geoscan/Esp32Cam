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
constexpr auto gsUtilityRevealType(R (T::*)(Ts...)) -> GsUtilityLogMember<R, T, Ts...>
{
	return {};
}

template <class T, class R, class ...Ts>
constexpr auto gsUtilityRevealType(R (*)(Ts...)) -> GsUtilityLogFunction<R, T, Ts...>
{
	return {};
}

#define GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) \
	typename decltype(gsUtilityRevealType< cls >(&cls::method))::Impl<&cls::method>

template <class T>
struct GsUtilityLogvMethod {
	static constexpr bool enabled = false;
};

#define GS_UTILITY_LOGV_METHOD_SET_ENABLED(cls, method, en) \
template<> \
struct GsUtilityLogvMethod< GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) > { \
	static constexpr bool enabled = en; \
};

#define GS_UTILITY_LOGV_METHOD(tag, cls, method, ...) \
struct cls##method ; \
do { \
	if (GsUtilityLogvMethod< GS_UTILITY_LOG_METHOD_MARKER_TYPE(cls, method) >::enabled) { \
		ESP_LOGV(tag, #cls "::" #method "() " __VA_ARGS__ ); \
	} \
} while (0)

//#define GS_UTILITY_LOGV_METHOD(tag, cls, method, ...) ESP_LOGV(tag, #cls "::" #method "() " __VA_ARGS__)

#endif // UTILITY_UTILITY_LOGSECTION_HPP_
