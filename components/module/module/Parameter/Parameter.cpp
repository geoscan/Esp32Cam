//
// Api.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "module/Parameter/ParameterDescription.hpp"
#include "module/Parameter/MemoryProvider.hpp"
#include "utility/al/String.hpp"
#include <array>
#include <algorithm>
#include <memory>
#include "Parameter.hpp"

namespace Mod {
namespace Par {

/// \brief Constexpr storage of parameter descriptions.
struct ParameterDescriptionStorage {
	static constexpr std::array<ParameterDescription, 1> kParameterDescriptions{{
		{
			0,  // id
			"WifiApSsid",  // name
			ParameterType::Str,
			Mod::Module::WifiAp,
			Mod::Fld::Field::StringIdentifier,
			MemoryProviderType::Sd,
		}
	}};

	/// \brief Max length of a parameter's string name. It is kept being equal 16, for compatibility w/ MAVLink
	static constexpr std::size_t kParameterStrlen = 16;
	/// \brief Compile-time validation of parameters' name length
	static constexpr bool validateStrlen(std::size_t pos = 0)
	{
		return pos >= kParameterDescriptions.size() ?
			true :
			Ut::Al::Str::cxStrlen(kParameterDescriptions[pos].name) <= kParameterStrlen && validateStrlen(pos + 1);
	}

	/// \brief Convert (MODULE, FIELD) pair to a parameter's unique identifier
	/// \returns True, if found. Sets `oId`
	static bool toId(Module module, Fld::Field field, std::size_t &oId)
	{
		bool res = false;
		auto it = std::find_if(std::begin(ParameterDescriptionStorage::kParameterDescriptions),
			std::end(ParameterDescriptionStorage::kParameterDescriptions),
			[module, field](const ParameterDescription &aDescription)
			{
				return module == aDescription.module && field == aDescription.field;
			});

		if (it != std::end(ParameterDescriptionStorage::kParameterDescriptions)) {
			res = true;
			oId = it->id;
		}

		return res;
	}

	static std::size_t size()
	{
		return kParameterDescriptions.size();
	}
};

static_assert(ParameterDescriptionStorage::validateStrlen(), "A parameter's name length has been exceeded");

/// \brief Static parameter instances storage.
struct InstanceStorage {
	std::array<std::unique_ptr<Parameter>, ParameterDescriptionStorage::kParameterDescriptions.size()> instances;

	/// \brief Makes an attempt to construct an instance. Returns, if already constructed
	void ensureAt(std::size_t id)
	{
		assert(id < instances.size());
	}

	/// \brief Returns an instance by index. `nullptr`, if the bounds have been
	/// exceeded
	Parameter *instanceById(std::size_t id)
	{
		Parameter *instance = nullptr;

		if (id < instances.size()) {
			ensureAt(id);
			instance = instances[id].get();
		}

		return instance;
	}
};

static InstanceStorage sInstanceStorage;

/// \brief Initializes and selects a memory provider using a parameter's
/// description
struct MemoryProviderStorage {
	std::unique_ptr<MemoryProvider> mSdMemoryProvider;
	std::unique_ptr<MemoryProvider> mNvsMemoryProvider;

	MemoryProvider &sdMemoryProvider()
	{
		assert(false);
		return *mSdMemoryProvider.get();
	}

	MemoryProvider &nvsMemoryProvider()
	{
		assert(false);
		return *mNvsMemoryProvider.get();
	}

	MemoryProvider *memoryProviderById(std::size_t id)
	{
		if (id < ParameterDescriptionStorage::size()) {
			switch (ParameterDescriptionStorage::kParameterDescriptions[id].memoryProviderType) {
				case MemoryProviderType::Nvs:
					return &nvsMemoryProvider();

				case MemoryProviderType::Sd:
					return &sdMemoryProvider();
			}
		}

		return nullptr;
	}
};

static MemoryProviderStorage sMemoryProviderStorage;

Parameter *Parameter::instanceByMf(Module module, Fld::Field field)
{
	std::size_t id;
	Parameter *instance = nullptr;

	if (ParameterDescriptionStorage::toId(module, field, id)) {
		instance = sInstanceStorage.instanceById(id);
	}

	return instance;
}

Parameter::Parameter(std::size_t aId) : mId{aId}
{
}

}  // namespace Par
}  // namespace Mod
