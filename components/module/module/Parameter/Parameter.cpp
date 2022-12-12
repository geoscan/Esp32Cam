//
// Api.cpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MODULE_DEBUG_LEVEL)
#include <esp_log.h>
#include "module/Variant.hpp"
#include "module/Parameter/ParameterDescription.hpp"
#include "module/Parameter/MemoryProvider.hpp"
#include "module/Parameter/MemoryProvider/SdMemoryProvider.hpp"
#include "module/module.hpp"
#include "utility/al/String.hpp"
#include "utility/LogSection.hpp"
#include <array>
#include <algorithm>
#include <memory>
#include "Parameter.hpp"
#include <mutex>

GS_UTILITY_LOGD_CLASS_ASPECT_SET_ENABLED(Mod::Par::Parameter, "tracing", 1);

namespace Mod {
namespace Par {

static constexpr const std::array<ParameterDescription, 5> kParameterDescriptions = {{
	// AP SSID (this AP)
	{
		0,  // id
		"WifiApSsid",  // name
		ParameterType::Str,
		Mod::Module::WifiAp,
		Mod::Fld::Field::StringIdentifier,
		MemoryProviderType::Sd,
		true,  // mirrorField
	},
	// AP password (this AP)
	{
		1,  // id
		"WifiApPassword",  // name
		ParameterType::Str,
		Mod::Module::WifiAp,
		Mod::Fld::Field::Password,
		MemoryProviderType::Sd,
		true,  // mirrorField
	},
	// STA SSID (remote AP)
	{
		2,  // id
		"WifiStaSsid",  // name
		ParameterType::Str,
		Mod::Module::WifiStaConnection,
		Mod::Fld::Field::StringIdentifier,
		MemoryProviderType::Sd,
		true,  // mirrorField
	},
	// STA password (remote AP)
	{
		3,  // id
		"WifiStaPassword",  // name
		ParameterType::Str,
		Mod::Module::WifiStaConnection,
		Mod::Fld::Field::Password,
		MemoryProviderType::Sd,
		true,  // mirrorField
	},
	{
		4,  //id
		"WifiStaAutoconn",
		ParameterType::I32,
		Mod::Module::WifiStaConnection,
		Mod::Fld::Field::Initialized,
		MemoryProviderType::Sd,
		true,  // mirrorField
	}
}};

/// \brief Max length of a parameter's string name. It is kept being equal 16, for compatibility w/ MAVLink
static constexpr std::size_t kParameterStrlen = 16;

/// \brief Compile-time validation of parameters' name length
static constexpr bool pardescValidateStrlen(std::size_t pos = 0)
{
	return pos >= kParameterDescriptions.size() ?
		true :
		Ut::Al::Str::cxStrlen(kParameterDescriptions[pos].name) <= kParameterStrlen && pardescValidateStrlen(pos + 1)
			&& Ut::Al::Str::cxStrlen(kParameterDescriptions[pos].name) > 0;
}

static constexpr bool pardescValidateUniquenessImpl(std::size_t currentId, std::size_t candidateId)
{
	return candidateId >= kParameterDescriptions.size() || currentId >= kParameterDescriptions.size() ? true :
		currentId == candidateId ? pardescValidateUniquenessImpl(currentId, candidateId + 1) :
		Ut::Al::Str::cxStrcpy(kParameterDescriptions[currentId].name, kParameterDescriptions[candidateId].name) != 0
			&& (kParameterDescriptions[currentId].field != kParameterDescriptions[candidateId].field
			|| kParameterDescriptions[currentId].module != kParameterDescriptions[candidateId].module)
			&& pardescValidateUniquenessImpl(currentId, candidateId + 1);
}

/// \brief Makes sure that each parameter description is unique
static constexpr bool pardescValidateUniqueness(std::size_t currentId = 0)
{
	return currentId >= kParameterDescriptions.size() ? true :
		pardescValidateUniquenessImpl(currentId, currentId) && pardescValidateUniqueness(currentId + 1);
}

static constexpr bool pardescEnsureSequence(std::size_t currentId = 0)
{
	return currentId >= kParameterDescriptions.size() ? true :
		currentId == kParameterDescriptions[currentId].id && pardescEnsureSequence(currentId + 1);
}

static_assert(pardescValidateStrlen(), "A parameter's name length constraints have been violated");
static_assert(pardescValidateUniqueness(), "Parameters must be unique");
static_assert(pardescEnsureSequence(), "There is a mismatch between some parameter's position and its id.");

/// \brief Convert (MODULE, FIELD) pair to a parameter's unique identifier
/// \arg [out] oId Id of the parameter for which a decsription has been found
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
		GS_UTILITY_LOGD_CLASS_ASPECT(Mod::kDebugTag, Parameter, "tracing",
			"found parameter, module=%d, field=%d, id=%d", static_cast<int>(module), static_cast<int>(field), it->id);
		res = true;
		oId = it->id;
	}

	return res;
}

/// \brief Static parameter instances storage.
struct InstanceStorage {
	std::array<std::unique_ptr<Parameter>, kParameterDescriptions.size()> instances;

	/// \brief Makes an attempt to construct an instance. Returns, if already constructed
	void ensureAt(std::size_t id)
	{
		assert(id < instances.size());

		if (instances[id].get() == nullptr) {
			instances[id] = std::unique_ptr<Parameter>{new Parameter{id}};
			ESP_LOGV(Mod::kDebugTag, "Parameter, InstanceStorage::ensureAt() created parameter id %d",
				instances[id]->id());
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
static std::mutex sMutex{};

const ParameterDescription *Parameter::descriptionByMf(Module module, Fld::Field field)
{
	std::size_t id;
	const ParameterDescription *ret = nullptr;

	if (pardescToId(module, field, id)) {
		ret = &kParameterDescriptions[id];
	}

	return ret;
}

Result Parameter::fetch()
{
	Result res = Result::Ok;
	MemoryProvider *memoryProvider = nullptr;
	{
		std::lock_guard<std::mutex> lock{sMutex};
		memoryProvider = sMemoryProviderStorage.memoryProviderById(id());
	}

	if (memoryProvider == nullptr) {
		res = Result::MemoryProviderNotFound;
		ESP_LOGW(Mod::kDebugTag, "Parameter::fetch: %s, parameter id %d", Par::resultAsStr(res), id());
	} else {
		std::lock_guard<std::mutex> lock{sMutex};
		res = memoryProvider->load(kParameterDescriptions[id()], variant);
		GS_UTILITY_LOGD_CLASS_ASPECT(Mod::kDebugTag, Parameter, "tracing", "Loading parameter, id=%d", id());
	}

	return res;
}

Result Parameter::commit()
{
	Result res = Result::Ok;
	MemoryProvider *memoryProvider = nullptr;
	{
		std::lock_guard<std::mutex> lock{sMutex};
		memoryProvider = sMemoryProviderStorage.memoryProviderById(id());
	}

	if (memoryProvider == nullptr) {
		res = Result::MemoryProviderNotFound;
		ESP_LOGW(Mod::kDebugTag, "Parameter::commit: %s, parameter id %d", Par::resultAsStr(res), id());
	} else {
		GS_UTILITY_LOGD_CLASS_ASPECT(Mod::kDebugTag, Parameter, "tracing", "Saving parameter, id=%d", id());
		std::lock_guard<std::mutex> lock{sMutex};
		res = memoryProvider->save(kParameterDescriptions[id()], variant);
	}

	return res;
}

Parameter *Parameter::instanceByMf(Module module, Fld::Field field)
{
	std::size_t id = 0;
	Parameter *instance = nullptr;

	if (pardescToId(module, field, id)) {
		std::lock_guard<std::mutex> lock{sMutex};
		instance = sInstanceStorage.instanceById(id);
	}

	return instance;
}

const std::string &Parameter::asStr() const
{
	assert(ParameterType::Str == kParameterDescriptions[id()].parameterType);

	return variant.get_unchecked<std::string>();
}

int32_t Parameter::asI32() const
{
	assert(ParameterType::I32 == kParameterDescriptions[id()].parameterType);

	return variant.get_unchecked<std::int32_t>();
}

Parameter::Parameter(std::size_t aId) : mId{aId}
{
}

}  // namespace Par
}  // namespace Mod
