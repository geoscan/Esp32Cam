//
// Crc32.hpp
//
// Created on:
//     Author: Kirill Dmitriev
//

#ifndef COMPONENTS_UTILITY_UTILITY_AL_CRC32_HPP
#define COMPONENTS_UTILITY_UTILITY_AL_CRC32_HPP

#include <stdint.h>

namespace Ut {
namespace Al {

struct Crc32 {
	static uint32_t calculateCrc32(const void *data, uint32_t len) {
		return updateCrc32(0, data, len);
	}

	static uint32_t updateCrc32(uint32_t cs, const void *buffer, uint32_t len);
};

}  // Al
}  // Ut

#endif  // COMPONENTS_UTILITY_UTILITY_AL_CRC32_HPP
