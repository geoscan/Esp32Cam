//
// Api.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_API_HPP_)
#define MODULE_MODULE_PARAMETER_API_HPP_

#include "utility/MakeSingleton.hpp"

namespace Mod {
namespace Par {

class Parameter;

class Api : public Ut::MakeSingleton<Api> {
public:
	Api();
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_API_HPP_
