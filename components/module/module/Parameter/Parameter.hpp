//
// Api.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_API_HPP_)
#define MODULE_MODULE_PARAMETER_API_HPP_

#include "module/Parameter/Result.hpp"
#include "module/Types.hpp"

namespace Mod {

class Variant;

namespace Par {

class MemoryProvider;
class ParameterDescription;

class Parameter {
public:
	friend class InstanceStorage;
	/// \brief Provides parameter description based on (MODULE_TYPE,
	/// FIELD_TYPE) pair
	/// \return nullptr, if not found
	static const ParameterDescription *descriptionByMf(Mod::Module, Mod::Fld::Field);
	virtual ~Parameter() = default;
	/// \brief Make an attempt to read the stored value from a storage
	Result fetch();
	/// \brief Make an attempt to write the stored value into a storage
	Result commit();
	/// \brief Gets an instance using (MODULE, FIELD) pair. Returns `nullptr`,
	/// if the pair is incorrect
	static Parameter *instanceByMf(Mod::Module module, Mod::Fld::Field);
	/// \brief Get the underlying string. If the parameter is not of
	/// "std::string" type, an assertion will be raised.
	const std::string &asStr() const;
	/// \brief Get the underlying 32-bit integer. If the parameter is not of
	/// "std::string" type, an assertion will be raised.
	std::int32_t asI32() const;

	/// \brief Initialize the parameter w/ a particular value
	/// \post The value will not be stored in the underlying non-volatile
	/// storage as a result of this call. To make it so, call `commit`
	inline void set(const Variant &aVariant)
	{
		variant = aVariant;
	}

	/// \brief Get the parameter's unique id
	inline std::size_t id() const
	{
		return mId;
	}
private:
	explicit Parameter(std::size_t aId);
	Parameter(const Parameter &) = default;
	Parameter(Parameter &&) = default;
	Parameter &operator=(const Parameter &) = default;
	Parameter &operator=(Parameter &&) = default;
private:
	std::size_t mId;
	Variant variant;
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_API_HPP_
