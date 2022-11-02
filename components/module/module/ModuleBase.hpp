//
// ModuleBase.hpp
//
// Created on: Apr 22, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

//
// Various routines for checking on system status, probably (depending on when you read it) changing it
//

#ifndef SUB_INCLUDE_SUB_SYS_HPP_
#define SUB_INCLUDE_SUB_SYS_HPP_

#include "Types.hpp"
#include <Rr/Util/Module.hpp>
#include <mutex>
#include <list>

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

	/// \brief A distributed module is identified w/ an enum entry.
	///
	/// \note A necessity to distinguish modules by their IDs may arise.
	inline Module getModule() const
	{
		return identity.type;
	}

	/// \brief Notifies subscribers upon a module field's change
	///
	/// \details Notifying subscribers is a responsibility of the module implementing the API.
	static void notifyFieldAsync(const Fld::ModuleField &moduleField);

	template <Module Im, Fld::Field If>
	using Field = typename Fld::GetType<If, Im>::Type;

	/// \brief Iterates through registered modules and forwards the request to update a field. The requestor is
	/// notified through use of a callback (`aCb`)
	template <Module Im, Fld::Field If>
	static void moduleFieldWriteIter(const typename Fld::GetType<If, Im>::Type &field, Fld::OnWriteResponseCallback aCb)
	{
		for (auto &mod : ModuleBase::getIterators()) {
			if (mod.getModule() == Im || Im == Module::All) {
				mod.setFieldValue({If, field}, aCb);
			}
		}
	}

	/// \brief Iterates through registered modules and forwards the request to get a field. The requestor is notified
	/// through use of a callback (`aCb`)
	template <Module Im, Fld::Field If>
	static void moduleFieldReadIter(std::function<void(typename Fld::GetType<If, Im>::Type)> aCb)
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
	/// \brief Helper constructor that initializes the "field variant" of a `Fld::Resp` instance.
	/// \sa `Mod::Fld::GetType`
	template <Module Im, Fld::Field If, class ...Ts>
	typename Fld::Resp makeResponse(Ts &&...aValue)
	{
		return typename Fld::Resp{typename Fld::template GetType<If, Im>::Type{std::forward<Ts>(aValue)...}};
	}

	/// \brief Asynchronous getter, subject to implementation.
	virtual void getFieldValue(Fld::Req aReq, Fld::OnResponseCallback aOnResponse);

	/// \brief Asynchronous setter, subject to implementation.
	virtual void setFieldValue(Fld::WriteReq aReq, Fld::OnWriteResponseCallback aCb);
private:
	struct {
		Module type;
	} identity;
};

}  // namespace Mod

#endif // SUB_INCLUDE_SUB_SYS_HPP_
