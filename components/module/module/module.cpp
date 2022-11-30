//
// module.hpp
//
// Created: 2022-08-10
//  Author: d.murashov
//

// Override debug level.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html#_CPPv417esp_log_level_setPKc15esp_log_level_t
#define LOG_LOCAL_LEVEL ((esp_log_level_t)CONFIG_MODULE_DEBUG_LEVEL)
#include <esp_log.h>
#include "module.hpp"
#include "module/Parameter/MemoryProvider.hpp"
#include "module/Parameter/MemoryProvider/SdMemoryProvider.hpp"
#include "module/Parameter/Parameter.hpp"
#include "utility/time.hpp"

namespace Mod {

/// \brief Debugging. Initializes the system for a test parameters read / write
static void initDemoPar();

void initDemoPar()
{
	auto *instance = Par::Parameter::instanceByMf(Mod::Module::WifiAp, Mod::Fld::Field::StringIdentifier);
	assert(instance != nullptr);
	auto res = instance->fetch();
	ESP_LOGI(Mod::kDebugTag, "module: got instance, id %d", instance->id());

	if (res == Mod::Par::Result::Ok) {
		ESP_LOGI(Mod::kDebugTag, "module: initDemoPar: successfully fetched the parameter, value: %s",
			instance->asStr().c_str());
	} else {
		ESP_LOGI(Mod::kDebugTag, "module: initDemoPar: failed to fetch the parameter, error %s",
			Mod::Par::resultAsStr(res));
		constexpr const char *kValue = "EchoHello";
		ESP_LOGI(Mod::kDebugTag, "module: got instance, id %d", instance->id());
		instance->set(kValue);
		ESP_LOGI(Mod::kDebugTag, "module: got instance, id %d", instance->id());
		ESP_LOGI(Mod::kDebugTag, "module: setting value %s, parameter id %d", kValue, instance->id());
		instance->commit();
	}
}

void init()
{
	esp_log_level_set(Mod::kDebugTag, (esp_log_level_t)CONFIG_MODULE_DEBUG_LEVEL);
	ESP_LOGD(Mod::kDebugTag, "Debug log test");
	ESP_LOGV(Mod::kDebugTag, "Verbose log test");
}

}  // namespace Mod
