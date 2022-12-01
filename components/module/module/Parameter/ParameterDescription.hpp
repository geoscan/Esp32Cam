//
// ParameterDescription.hpp
//
// Created on: Nov 24, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_PARAMETERDESCRIPTION_HPP_)
#define MODULE_MODULE_PARAMETER_PARAMETERDESCRIPTION_HPP_

#include "module/Types.hpp"
#include <cstdint>

namespace Mod {
namespace Par {

enum class ParameterType {
	Str,
	I32,
};

/// \brief Defines the type of a non-volatile provider
enum class MemoryProviderType {
	Sd,  // SD card
	Nvs,  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
};

/// \brief Description of a parameter that can be used to save it into a
/// non-volatile storage
struct ParameterDescription {
	/// \brief Perpetual identifier. Must not be changed between builds
	std::uint32_t id;
	/// \brief Human-readable name
	const char *name;
	ParameterType parameterType;
	// Just like field, a parameter is uniquely identified by a (MODULE, FIELD)
	// pair. Also, each parameter is associated w/ a module.
	Mod::Module module;
	Mod::Fld::Field field;
	/// \brief Physical medium which will store the value
	MemoryProviderType memoryProviderType;
	/// \brief If set, changing a module's field will also lead to an attempt
	/// to change the parameter. Otherwise, the parameter will have to be
	/// changed manually
	bool mirrorField;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_PARAMETERDESCRIPTION_HPP_
