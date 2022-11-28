//
// Variant.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_PARAMETERVALUESTORAGE_HPP_)
#define MODULE_MODULE_PARAMETER_PARAMETERVALUESTORAGE_HPP_

#include <mapbox/variant.hpp>
#include <string>

namespace Mod {
namespace Par {

using VariantBase = mapbox::util::variant<std::string, std::int32_t>;

class Variant : public VariantBase {
	using VariantBase::VariantBase;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PARAMETERVALUESTORAGE_HPP_
