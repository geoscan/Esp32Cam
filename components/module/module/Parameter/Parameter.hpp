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

namespace Mod {
namespace Par {

class MemoryProvider;

class Parameter : public Variant {
public:
	/// \brief Make an attempt to load the value from a storage
	Result fetch();
	/// \brief Make an attempt to load the value into a storage
	Result commit();
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
	static Parameter *instanceByMf(Mod::Module module, Mod::Fld::Field);
private:
	Parameter(std::size_t aId);
private:
	std::size_t mId;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_API_HPP_
