//
// FlashMemory.hpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_ZD35_ZD35_FLASHMEMORY_HPP_
#define COMPONENTS_ZD35_ZD35_FLASHMEMORY_HPP_

#include "system/middleware/FlashMemory.hpp"
#include <esp_flash.h>

namespace Zd35 {

/// \brief Implmements `Sys::FlashMemory` API
class FlashMemory : public Sys::FlashMemory {
public:
	FlashMemory(esp_flash_t *aEspFlash);
	virtual Sys::Error writeBlocks(std::uint32_t aWriteBlockOffset, const std::uint8_t *aData,
		std::size_t aDataLength) override;
	virtual Sys::Error readBlocks(std::uint32_t aReadBlockOffset, std::uint32_t anReadBlocks, std::uint8_t *aBuffer,
		std::size_t aBufferSize) override;
	virtual Sys::Error eraseBlocks(std::uint32_t aEraseBlockOffset, std::uint32_t anBlocks) = 0;

	~FlashMemory() override = default;

private:
	esp_flash_t *espFlash;
};

}  // Zd35

#endif  // COMPONENTS_ZD35_ZD35_FLASHMEMORY_HPP_
