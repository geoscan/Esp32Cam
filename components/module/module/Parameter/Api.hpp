//
// Api.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_API_HPP_)
#define MODULE_MODULE_PARAMETER_API_HPP_

#include "utility/MakeSingleton.hpp"
#include "module/Parameter/Result.hpp"
#include "module/Types.hpp"
#include <memory>

namespace Mod {
namespace Par {

class Variant;
class MemoryProvider;

class Api : public Ut::MakeSingleton<Api> {
private:
	/// \brief Stores memory providers, performs initialization, and retuns an
	/// appropriate instance based on the description provided.
	struct MemoryProviderSelector {
		std::unique_ptr<MemoryProvider> sdMemoryProvider;
		std::unique_ptr<MemoryProvider> nvsMemoryProvider;
		/// \brief Returns the appropriate memory provider
		MemoryProvider &match(std::size_t parameterId);
	};
public:
	Api();
	/// \brief Set a parameter's value identifying it by its Module and Field indices
	Mod::Par::Result setValue(Mod::Module module, Mod::Fld::Field field, const Variant &value);
private:
	/// \brief Convert (MODULE, FIELD) pair to a parameter's unique identifier
	/// \returns True, if found. Sets `oId`
	bool toId(Mod::Module module, Mod::Fld::Field field, std::size_t &oId);
private:
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_API_HPP_
