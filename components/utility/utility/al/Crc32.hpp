//
// Crc32.hpp
//
// Created on:
//     Author: Kirill Dmitriev
//

#ifndef COMPONENTS_UTILITY_UTILITY_AL_CRC32_HPP
#define COMPONENTS_UTILITY_UTILITY_AL_CRC32_HPP

#include <cstdint>

namespace Ut {
namespace Al {

struct Crc32 {
	static std::uint32_t calculateCrc32(const void *data, std::uint32_t len)
	{
		return updateCrc32(0, data, len);
	}

	static std::uint32_t updateCrc32(uint32_t cs, const void *buffer, std::uint32_t len);
};

}  // Al
}  // Ut

#endif  // COMPONENTS_UTILITY_UTILITY_AL_CRC32_HPP
