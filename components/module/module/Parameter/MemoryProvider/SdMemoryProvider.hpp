//
// SdMemoryProvider.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_MEMORYPROVIDER_HPP_)
#define MODULE_MODULE_PARAMETER_MEMORYPROVIDER_HPP_

#include "module/Parameter/MemoryProvider.hpp"

namespace Mod {
namespace Par {

/// \brief Wrapper over FAT / SPI SD card. Loads / stores parameter from json
/// using standard C file library. Performs (de)initialization of SD storage.
class SdMemoryProvider : public MemoryProvider {
public:
	Result load(const ParameterDescription &, Variant &) override;
	Result save(const ParameterDescription &, const Variant &value) override;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_MEMORYPROVIDER_HPP_
