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
	All,  ///< The request is addressed to every module
};

struct None {};

template <class T>
struct ModApi {
	template <template <T, Module> class TgetResponseType, class ...TsResponseVariants>
	struct Response {
		using Variant = mapbox::util::variant< None, TsResponseVariants...>;

		template <T It, Module Im>
		using Type = typename TgetResponseType<It, Im>::Type;

		Variant variant;  ///< The actual response
		Module module;  ///< Type of the module producing this response

		template <Module Im, T If, class Val>
		bool tryGet(Val &aOut)
		{
			bool ret = false;

			if (Utility::Algorithm::in(Im, module, Module::All)) {
				variant.match(
					[&ret, &aOut](const typename TgetResponseType<If, Im>::Type &aVal) {
						aOut = aVal;
						ret = true;
					},
					[](...){}
				);
			}

			return ret;
		}
	};
};

namespace Fld {

enum class Field : std::uint8_t {
	FrameSize,
	VendorName,
	ModelName,
	Initialized,
};

template <class T>
using StoreType = typename Rr::Trait::StoreType<T>;

template <Field, Module=Module::All> struct GetType : StoreType<None> {};
template <> struct GetType<Field::FrameSize, Module::Camera> : StoreType<std::pair<int, int>> {};
template <Module I> struct GetType<Field::Initialized, I> : StoreType<bool> {};
template <Module I> struct GetType<Field::VendorName, I> : StoreType<const char *> {};
template <Module I> struct GetType<Field::ModelName, I> : StoreType<const char *> {};

using Resp = ModApi<Field>::Response<GetType,
	typename GetType<Field::FrameSize, Module::Camera>::Type,
	typename GetType<Field::Initialized>::Type,
	typename GetType<Field::VendorName>::Type,
	typename GetType<Field::ModelName>::Type>;

struct Req {
	Module module;  ///< Requested module
	Field field;  ///< Requested field

	bool shouldRespond(Module aThisModule);
};

using ModuleGetField = typename Sub::NoLockKey<Resp(Req)>;  ///< \pre NoLockKey implies that the module must ensure its MT-safety
using ModuleCb = decltype(*ModuleGetField::getIterators().begin());

template <Module Im, Field If, class Val>
bool moduleCbTryGet(ModuleCb &aCb, Val &aOut)
{
	return aCb({Im, If}).tryGet<Im, If>(aOut);
}

}  // namespace Fld
}  // namespace Sys
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_SYS_HPP_
