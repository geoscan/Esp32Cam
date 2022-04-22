//
// Sys.hpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

//
// Various routines for checking on system status, probably (depending on when you read it) changing it
//

#ifndef SUB_INCLUDE_SUB_SYS_HPP_
#define SUB_INCLUDE_SUB_SYS_HPP_

#include "sub/Subscription.hpp"
#include "utility/Algorithm.hpp"
#include <cstdint>
#include <mapbox/variant.hpp>
#include <Rr/Trait/StoreType.hpp>
#include <utility>

namespace Sub {
namespace Sys {

enum class Module : std::uint8_t {
	Camera,
	Generic,
};

namespace Fld {

enum class Field : std::uint8_t {
	FrameSize,
	Initialized,
};

inline std::uint16_t asNumeric(Module module, Field field)
{
	std::uint16_t res = 0;
	res = (static_cast<std::uint16_t>(module) << sizeof(Module)) | static_cast<std::uint16_t>(field);

	return res;
}

template <Module Im, Field If>
constexpr std::uint16_t asNumeric()
{
	return static_cast<std::uint16_t>(Im) << sizeof(Module) | static_cast<std::uint16_t>(If);
}

template <class T>
using StoreType = typename Rr::Trait::StoreType<T>;

struct None {
};

template <Field, Module=Module::Generic> struct GetType : StoreType<None> {};
template <> struct GetType<Field::FrameSize, Module::Camera> : StoreType<std::pair<int, int>> {};
template <Module I> struct GetType<Field::Initialized, I> : StoreType<bool> {};

struct Resp {
	using ResponseVariant = mapbox::util::variant<
		None,
		typename GetType<Field::FrameSize, Module::Camera>::Type,
		typename GetType<Field::Initialized>::Type>;

	ResponseVariant responseVariant;  ///< The actual response
	Module module;  ///< Type of the module producing this response

	template <Module Im, Field If>
	bool tryGet(typename GetType<If, Im>::Type &aOut)
	{
		bool ret = false;

		if (Utility::Algorithm::in(module, Im, Module::Generic)) {
			responseVariant.match(
				[&ret, &aOut](const typename GetType<If, Im>::Type &aVal) {
					aOut = aVal;
				},
				[](...){}
			);
		}

		return ret;
	}
};

struct Req {
	Module module;  ///< Requested module
	Field field;  ///< Requested field
};

using ModuleGetField = Sub::NoLockKey<Resp(Req)>;  ///< \pre NoLockKey implies that the module must ensure its MT-safety

}  // namespace Fld
}  // namespace Sys
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_SYS_HPP_
