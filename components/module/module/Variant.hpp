//
// Variant.hpp
//
// Created on: Nov 30, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_VARIANT_HPP_)
#define MODULE_MODULE_VARIANT_HPP_

#include <utility>
#include <cstdint>
#include <asio.hpp>
#include <array>
#include <mapbox/variant.hpp>

namespace Mod {

struct None{};

/// \brief Variant type serving as the storage module fields' values
using VariantBase = typename mapbox::util::variant< None, unsigned, std::pair<int, int>, bool, const char *,
	mapbox::util::variant<asio::ip::address_v4>, std::tuple<std::uint8_t, const char *>, std::uint8_t,
	std::array<std::uint16_t, 4>, std::string, std::int32_t>;

struct Variant : VariantBase {
	using VariantBase::VariantBase;
};

}  // namespace Mod

#endif // MODULE_MODULE_VARIANT_HPP_
