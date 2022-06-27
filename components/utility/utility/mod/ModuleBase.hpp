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
#include <Rr/Util/Module.hpp>
#include <mutex>
#include <list>

namespace Utility {
namespace Mod {

class ModuleBase;

namespace ModuleBaseImpl {

/// \brief Defines access policy
///
struct SyncTrait {
	using Mutex = std::mutex;
	static constexpr auto kStoragePolicy = Rr::Sync::Policy::Type::Mutex;
	static constexpr auto kSharedAccessPolicy = Rr::Sync::Policy::Type::None;
};

using ModuleRegistry = Rr::Util::ModuleRegistry<ModuleBase, ModuleBaseImpl::SyncTrait, std::list>;

}  // namespace ModuleBaseImpl

/// \brief A detachable entity that stores its state in the form of fields of various types.
///
/// A module may unify an entire set of CPP modules, even if those are scattered across the project.
///
class ModuleBase :
	public ModuleBaseImpl::ModuleRegistry,
	public Rr::Util::MakeModule<typename ModuleBaseImpl::ModuleRegistry>
{
public:
	ModuleBase(Module aModule);
	virtual ~ModuleBase() = default;
	Module getModule() const;

	template <Module Im, Fld::Field If>
	using Field = typename Fld::GetType<If, Im>::Type;

	template <Module Im, Fld::Field If, class Tcb>
	static void moduleFieldReadIter(Tcb &&aCb)
	{
		for (auto &mod : ModuleBase::getIterators()) {
			if (mod.getModule() == Im || Im == Module::All) {
				mod.getFieldValue(
					{If},
					[aCb](typename Fld::Resp aResp)
					{
						using Ft = Field<Im, If>;
						aCb(aResp.variant.template get_unchecked<Ft>());
					}
				);
			}
		}
	}

protected:
	template <Module Im, Fld::Field If, class ...Ts>
	typename Fld::Resp makeResponse(Ts &&...aValue)
	{
		return typename Fld::Resp{typename Fld::template GetType<If, Im>::Type{std::forward<Ts>(aValue)...}};
	}

	virtual void getFieldValue(Fld::Req aReq, Fld::OnResponseCallback aOnResponse);

private:
	struct {
		Module type;
	} identity;
};

}  // namespace Mod
}  // namespace Utility

#endif // SUB_INCLUDE_SUB_SYS_HPP_
