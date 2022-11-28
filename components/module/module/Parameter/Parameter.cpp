//
// Api.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "module/Parameter/ParameterDescription.hpp"
#include "module/Parameter/MemoryProvider.hpp"
#include "module/Parameter/MemoryProvider/SdMemoryProvider.hpp"
#include "utility/al/String.hpp"
#include <array>
#include <algorithm>
#include <memory>
#include "Parameter.hpp"

namespace Mod {
namespace Par {

static constexpr const std::array<ParameterDescription, 1> kParameterDescriptions = {{
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
static constexpr bool pardescValidateStrlen(std::size_t pos = 0)
{
	return pos >= kParameterDescriptions.size() ?
		true :
		Ut::Al::Str::cxStrlen(kParameterDescriptions[pos].name) <= kParameterStrlen && pardescValidateStrlen(pos + 1);
}

/// \brief Convert (MODULE, FIELD) pair to a parameter's unique identifier
/// \returns True, if found. Sets `oId`
static bool pardescToId(Module module, Fld::Field field, std::size_t &oId)
{
	bool res = false;
	auto it = std::find_if(std::begin(kParameterDescriptions),
		std::end(kParameterDescriptions),
		[module, field](const ParameterDescription &aDescription)
		{
			return module == aDescription.module && field == aDescription.field;
		});

	if (it != std::end(kParameterDescriptions)) {
		res = true;
		oId = it->id;
	}

	return res;
}

static_assert(pardescValidateStrlen(), "A parameter's name length has been exceeded");

/// \brief Static parameter instances storage.
struct InstanceStorage {
	std::array<std::unique_ptr<Parameter>, kParameterDescriptions.size()> instances;

	/// \brief Makes an attempt to construct an instance. Returns, if already constructed
	void ensureAt(std::size_t id)
	{
		assert(id < instances.size());

		if (!instances[id]) {
			instances[id] = std::unique_ptr<Parameter>{new Parameter{id}};
		}
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

	MemoryProvider *sdMemoryProvider()
	{
		if (!mSdMemoryProvider) {
			mSdMemoryProvider = std::unique_ptr<MemoryProvider>{new SdMemoryProvider{}};
		}

		return mSdMemoryProvider.get();
	}

	MemoryProvider *nvsMemoryProvider()
	{
		return mNvsMemoryProvider.get();
	}

	MemoryProvider *memoryProviderById(std::size_t id)
	{
		if (id < kParameterDescriptions.size()) {
			switch (kParameterDescriptions[id].memoryProviderType) {
				case MemoryProviderType::Nvs:
					return nvsMemoryProvider();

				case MemoryProviderType::Sd:
					return sdMemoryProvider();
			}
		}

		return nullptr;
	}
};

static MemoryProviderStorage sMemoryProviderStorage;

Result Parameter::fetch()
{
	MemoryProvider *memoryProvider = sMemoryProviderStorage.memoryProviderById(id());
	Result res = Result::Ok;

	if (memoryProvider == nullptr) {
		res = Result::MemoryProviderNotFound;
	} else {
		res = memoryProvider->load(kParameterDescriptions[id()], *this);
	}

	return res;
}

Result Parameter::commit()
{
	MemoryProvider *memoryProvider = sMemoryProviderStorage.memoryProviderById(id());
	Result res = Result::Ok;

	if (memoryProvider == nullptr) {
		res = Result::MemoryProviderNotFound;
	} else {
		res = memoryProvider->save(kParameterDescriptions[id()], *this);
	}

	return res;
}

Parameter *Parameter::instanceByMf(Module module, Fld::Field field)
{
	std::size_t id;
	Parameter *instance = nullptr;

	if (pardescToId(module, field, id)) {
		instance = sInstanceStorage.instanceById(id);
	}

	return instance;
}

Parameter::Parameter(std::size_t aId) : mId{aId}
{
}

}  // namespace Par
}  // namespace Mod
