//
// NvsWrap.hpp
//
// Created on: Aug 18, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILTIY_UTILITY_SYSTEM_NVSWRAP_HPP_)
#define UTILTIY_UTILITY_SYSTEM_NVSWRAP_HPP_

#include <nvs_handle.hpp>
#include "utility/system/sys.hpp"
#include <esp_log.h>

namespace Ut {
namespace Sys {

template <class T>
esp_err_t nvsGet(const char *aStorage, const char *aKey, T &aOut)
{
	esp_err_t err = ESP_OK;
	auto nvsHandle = nvs::open_nvs_handle(aStorage, NVS_READONLY, &err);

	if (ESP_OK == err) {
		err = nvsHandle->get_item(aKey, aOut);

		if (ESP_OK != err) {
			ESP_LOGE(kTag, "Error \"%s\" when reading value \"%s\" from storage \"%s\" ", esp_err_to_name(err), aKey,
				aStorage);
		}
	} else {
		ESP_LOGE(kTag, "Error \"%s\" when opening NVS storage \"%s\" value \"%s\" (readonly)", aStorage, aKey,
			esp_err_to_name(err));
	}

	return err;
}

template <class T>
esp_err_t nvsSet(const char *aStorage, const char *aKey, T &&aOut)
{
	esp_err_t err = ESP_OK;
	auto nvsHandle = nvs::open_nvs_handle(aStorage, NVS_READWRITE, &err);

	if (ESP_OK == err) {
		err = nvsHandle->set_item(aKey, aOut);

		if (ESP_OK != err) {
			ESP_LOGE(kTag, "Error \"%s\" when writing value \"%s\" to storage \"%s\" ", esp_err_to_name(err), aKey,
				aStorage);
		}
	} else {
		ESP_LOGE(kTag, "Error \"%s\" when opening NVS storage \"%s\" value \"%s\" (read / write)", aStorage, aKey,
			esp_err_to_name(err));
	}

	return err;
}

}  // namespace Sys
}  // namespace Ut

#endif // UTILTIY_UTILITY_SYSTEM_NVSWRAP_HPP_
