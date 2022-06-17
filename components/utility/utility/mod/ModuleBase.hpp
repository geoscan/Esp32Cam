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

#include "utility/mod/Types.hpp"

namespace Utility {
namespace Mod {

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

}  // namespace Mod
}  // namespace Utility

#endif // SUB_INCLUDE_SUB_SYS_HPP_
