//
// Tracking.hpp
//
// Created on: Oct 31, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(SUB_INCLUDE_SUB_MODULE_HPP_)
#define SUB_INCLUDE_SUB_MODULE_HPP_

#include "sub/Subscription.hpp"
#include "module/Types.hpp"

namespace Sub {
namespace Mod {

/// \brief Stores the value of an updated field
struct ModuleFieldSetUpdate {
	Mod::Module module;
	Mod::Fld::Field field;
	Mod::Fld::FieldVariant variant;

	template <Mod::Module M, Mod::Fld::Field F>
	const inline typename Mod::Fld::GetType<F, M>::Type &getUnchecked()
	{
		return variant.template getUnchecked<M, F>();
	}
};

/// \brief A part of module API. This event gets triggered each time a module's field is successfully updated as a
/// result of a set request. \sa `Mod::ModuleBase`.
using OnModuleFieldSetUpdate = IndKey<void(ModuleFieldSetUpdate)>;

}  // namespace Mod
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_MODULE_HPP_
