//
// Parameter.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_PARAMETER_HPP_)
#define MODULE_MODULE_PARAMETER_PARAMETER_HPP_

namespace Mod {
namespace Par {

class MemoryProvider;
class Variant;

/// \brief An encapulation of a value stored in a permanent storage (SD card or
/// NVS)
class Parameter {
public:
	void setMemoryProvider(MemoryProvider &);
	/// \brief Save the current value into non-volatile storage
	void commit();
	void set(const Variant &);
private:
	MemoryProvider *memoryProvider;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PARAMETER_HPP_
