//
// Result.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_RESULT_HPP_)
#define MODULE_MODULE_PARAMETER_RESULT_HPP_

namespace Mod {
namespace Par {

enum class Result {
	/// No error has occured
	Ok,
	MemoryProviderNotFound,
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_RESULT_HPP_
