//
// MemoryProvider.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_PROVIDER_HPP_)
#define MODULE_MODULE_PARAMETER_PROVIDER_HPP_

#include "module/Parameter/Result.hpp"

namespace Mod {
namespace Par {

class ParameterDescription;
class Variant;

/// \brief Provides an API to a non-volatile storage (SD card or NVS)
class MemoryProvider {
public:
	/// \brief Make an attempt to load value from a non-volatile storage
	virtual Result load(const ParameterDescription &, Variant &) = 0;
	/// \brief Make an attempt to store value into a non-volatile storage
	virtual Result save(const ParameterDescription &, const Variant &value) = 0;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PROVIDER_HPP_
