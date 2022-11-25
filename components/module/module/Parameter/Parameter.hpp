//
// Api.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_API_HPP_)
#define MODULE_MODULE_PARAMETER_API_HPP_

#include "module/Parameter/Result.hpp"
#include "module/Parameter/Variant.hpp"
#include "module/Types.hpp"
#include <memory>

namespace Mod {
namespace Par {

class MemoryProvider;

class Parameter : public Variant {
public:
	void fetch();
	void commit();
	inline std::size_t id() const
	{
		return mId;
	}
	Parameter(const Parameter &) = delete;
	Parameter(Parameter &&) = delete;
	Parameter &operator=(const Parameter &) = delete;
	Parameter &operator=(Parameter &&) = delete;
	/// \brief Gets an instance using (MODULE, FIELD) pair. Returns `nullptr`,
	/// if the pair is incorrect
	Parameter *instance(Mod::Module module, Mod::Fld::Field);
private:
	/// \brief Convert (MODULE, FIELD) pair to a parameter's unique identifier
	/// \returns True, if found. Sets `oId`
	static bool toId(Mod::Module module, Mod::Fld::Field field, std::size_t &oId);
	static MemoryProvider &memoryProvider(std::size_t id);
private:
	std::size_t mId;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_API_HPP_
