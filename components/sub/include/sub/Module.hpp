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

/// \brief A part of module API. This event gets triggered each time a module's field is successfully updated as a
/// result of a set request. \sa `Mod::ModuleBase`.
using OnModuleFieldUpdate = IndKey<void(const ::Mod::Fld::ModuleField &)>;

}  // namespace Mod
}  // namespace Sub

#endif // SUB_INCLUDE_SUB_MODULE_HPP_
