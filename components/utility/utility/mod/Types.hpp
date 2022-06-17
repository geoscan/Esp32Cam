//
// Types.hpp
//
// Created on: Jun 17, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_MOD_TYPES_HPP_)
#define UTILITY_UTILITY_MOD_TYPES_HPP_

#include "sub/Subscription.hpp"
#include "utility/Algorithm.hpp"
#include <cstdint>
#include <mapbox/variant.hpp>
#include <Rr/Trait/StoreType.hpp>
#include <utility>
#include <functional>

namespace Utility {
namespace Mod {

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
				ret = true;
				variant.match(
					[&ret, &aOut, this](const typename TgetResponseType<If, Im>::Type &aVal) {
						aOut = aVal;
						ret = true;
					},
					[](...){}
				);
			}

			return ret;
		}

		///
		/// \return Check whether \arg aModule is addressed by `module` field
		///
		inline bool moduleMatch(Module aModule)
		{
			return Utility::Algorithm::in(module, aModule, Module::All);
		}
	};
};

namespace Fld {

enum class Field : std::uint8_t {
	FrameSize,
	VendorName,
	ModelName,
	Initialized,
	CaptureCount,  ///< Number of frames captured by a camera
	Recording,  ///< Whether a recording process is ongoing
};

template <class T>
using StoreType = typename Rr::Trait::StoreType<T>;

template <Field, Module=Module::All> struct GetType : StoreType<None> {};  ///< Type selector
template <> struct GetType<Field::FrameSize, Module::Camera> : StoreType<std::pair<int, int>> {};
template <Module I> struct GetType<Field::Initialized, I> : StoreType<bool> {};
template <Module I> struct GetType<Field::VendorName, I> : StoreType<const char *> {};
template <Module I> struct GetType<Field::ModelName, I> : StoreType<const char *> {};
template <> struct GetType<Field::CaptureCount, Module::Camera> : StoreType<unsigned> {};
template <> struct GetType<Field::Recording, Module::Camera> : StoreType<bool> {};

/// \brief Response variant type.
using Resp = ModApi<Field>::Response<
	// Selector to use
	GetType,
	// Variant types
	typename GetType<Field::CaptureCount, Module::Camera>::Type,
	typename GetType<Field::FrameSize, Module::Camera>::Type,
	typename GetType<Field::Initialized>::Type,
	typename GetType<Field::VendorName>::Type,
	typename GetType<Field::ModelName>::Type,
	typename GetType<Field::Recording, Module::Camera>::Type
>;

struct Req {
	Module module;  ///< Requested module
	Field field;  ///< Requested field

	bool shouldRespond(Module aThisModule);
};

using OnResponseCallback = typename std::function<void(Resp)>;
using ModuleGetFieldMult = typename ::Sub::NoLockKey<void(Req, OnResponseCallback)>;  ///< \pre NoLockKey implies that the module must ensure its MT-safety
using FieldType = Field;  /// Temp. alias. `Field` will be subjected to refactoring

}  // namespace Fld

using ModuleType = Module;  /// Temp. alias. `Module` will be subjected to refactoring.


}  // namespace Mod
}  // namespace Utility

#endif // UTILITY_UTILITY_MOD_TYPES_HPP_
