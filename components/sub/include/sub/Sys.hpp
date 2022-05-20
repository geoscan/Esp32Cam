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
#include <functional>

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
};

template <class T>
using StoreType = typename Rr::Trait::StoreType<T>;

template <Field, Module=Module::All> struct GetType : StoreType<None> {};
template <> struct GetType<Field::FrameSize, Module::Camera> : StoreType<std::pair<int, int>> {};
template <Module I> struct GetType<Field::Initialized, I> : StoreType<bool> {};
template <Module I> struct GetType<Field::VendorName, I> : StoreType<const char *> {};
template <Module I> struct GetType<Field::ModelName, I> : StoreType<const char *> {};
template <> struct GetType<Field::CaptureCount, Module::Camera> : StoreType<unsigned> {};

using Resp = ModApi<Field>::Response<GetType,
	typename GetType<Field::CaptureCount, Module::Camera>::Type,
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
using ModuleGetFieldMult = typename Sub::NoLockKey<void(Req, std::function<void(Resp)>)>;
using ModuleCb = decltype(*ModuleGetField::getIterators().begin());

template <Module Im, Field If, class Val>
bool moduleCbTryGet(ModuleCb &aCb, Val &aOut)
{
	return aCb({Im, If}).tryGet<Im, If>(aOut);
}

template <class Tcb>
inline void modulesVisitIterate(Req aReq, Tcb &&aCb)
{
	for (auto &cb : ModuleGetField::getIterators()) {
		cb(aReq).variant.match(
			[](...){},
			std::forward<Tcb>(aCb));
	}
}

using FieldType = Field;  /// Temp. alias. `Field` will be subjected to refactoring

}  // namespace Fld

using ModuleType = Module;  /// Temp. alias. `Module` will be subjected to refactoring.

/// \brief A detachable entity that stores its state in the form of fields of various types.
///
/// A module may unify an entire set of CPP modules, even if those are scattered across the project.
///
class ModuleBase {
public:
	ModuleBase(ModuleType aModuleType);
	virtual ~ModuleBase() = default;
	ModuleType getModuleType() const;

	template <ModuleType Im, Fld::FieldType If>
	using FieldType = typename Fld::GetType<If, Im>::Type;

	static void moduleFieldReadIter(typename Fld::ModuleGetFieldMult::Arg<0>,
		typename Fld::ModuleGetFieldMult::Arg<1>);

	template <ModuleType Im, Fld::FieldType If, class Tcb>
	static void moduleFieldReadIter(Tcb &&aCb)
	{
		for (auto &cb : Fld::ModuleGetFieldMult::getIterators()) {
			cb({Im, If},
				[aCb](typename Fld::Resp aResp)
				{
					using Ft = FieldType<Im, If>;
					if (aResp.moduleMatch(Im)) {
						aCb(aResp.variant.template get_unchecked<Ft>());
					}
				});
		}
	}

protected:
	template <ModuleType Im, Fld::FieldType If, class ...Ts>
	typename Fld::Resp makeResponse(Ts &&...aValue)
	{
		return typename Fld::Resp{
			typename Fld::template GetType<If, Im>::Type{std::forward<Ts>(aValue)...},
			Im};
	}

	virtual typename Fld::ModuleGetFieldMult::Ret getFieldValue(typename Fld::ModuleGetFieldMult::Arg<0>,
		typename Fld::ModuleGetFieldMult::Arg<1>);
private:
	typename Fld::ModuleGetFieldMult::Ret getFieldValueIpc(typename Fld::ModuleGetFieldMult::Arg<0>,
		typename Fld::ModuleGetFieldMult::Arg<1>);

private:
	struct {
		ModuleType type;
	} identity;

	struct {
		Fld::ModuleGetFieldMult fldModuleGetField;
	} key;
};

}  // namespace Sys
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_SYS_HPP_
