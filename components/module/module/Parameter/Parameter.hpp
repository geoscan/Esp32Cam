//
// Parameter.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_PARAMETER_HPP_)
#define MODULE_MODULE_PARAMETER_PARAMETER_HPP_

#include <cstdint>

namespace Mod {
namespace Par {

class MemoryProvider;
class Variant;

/// \brief An encapulation of a value stored in a permanent storage (SD card or
/// NVS)
class Parameter : public Variant {
public:
	Parameter(std::size_t id, MemoryProvider &memoryProvider);
	/// \brief Save the current value into non-volatile storage
	void commit();
private:
	MemoryProvider &memoryProvider;
	std::size_t mId;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PARAMETER_HPP_
