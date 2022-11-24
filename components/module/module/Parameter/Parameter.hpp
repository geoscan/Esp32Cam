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

/// \brief
class Parameter {
public:
	void setMemoryProvider(MemoryProvider &);
private:
	MemoryProvider *memoryProvider;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PARAMETER_HPP_
