//
// WifiConfigAp.cpp
//
// Created on: Dec 06, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "WifiConfigAp.hpp"
#include <algorithm>
#include <mbedtls/md5.h>
#include <array>
#include <cstdint>

namespace Mav {
namespace Hlpr {

void WifiConfigAp::ssidFillZero()
{
	std::fill_n(ssid, sizeof(ssid), 0);
}

void WifiConfigAp::passwordFillZero()
{
	std::fill_n(password, sizeof(password), 0);
}

void WifiConfigAp::passwordIntoMd5Stringify()
{
	const std::string passwordCache{password, sizeof(password)};
	constexpr std::size_t kMd5DigestLength = 16;
	std::array<std::uint8_t, kMd5DigestLength> digest{{0}};
	// Calculate MD5 digest as per the standard
	mbedtls_md5_context mbedtlsMd5Context;
	mbedtls_md5_init(&mbedtlsMd5Context);
	mbedtls_md5_starts_ret(&mbedtlsMd5Context);
	mbedtls_md5_update_ret(&mbedtlsMd5Context, reinterpret_cast<const std::uint8_t *>(passwordCache.data()),
		strlen(passwordCache.data()));
	// Dump the digest into the returned message
	mbedtls_md5_finish_ret(&mbedtlsMd5Context, digest.data());

	for (std::size_t i = 0; i < digest.size(); ++i) {
		snprintf(password + i * 2, sizeof(password) - i * 2, "%02x", digest[i]);
	}
}

}  // namespace Hlpr
}  // namespace Mav
