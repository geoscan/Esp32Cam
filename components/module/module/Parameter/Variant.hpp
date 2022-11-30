//
// Variant.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_PARAMETERVALUESTORAGE_HPP_)
#define MODULE_MODULE_PARAMETER_PARAMETERVALUESTORAGE_HPP_

#include "module/Types.hpp"
#include <mapbox/variant.hpp>
#include <string>

namespace Mod {
namespace Par {

struct Variant : public Mod::Fld::Variant {
	using Fld::Variant::Variant;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PARAMETERVALUESTORAGE_HPP_
