//
// FlashMemory.hpp
//
// Created on: Aug 18, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORY_HPP
#define COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORY_HPP

#include "system/Error.hpp"
#include "system/middleware/FlashMemoryGeometry.hpp"
#include <cstdint>

namespace Sys {

/// Represents a flash memory device as a set of of sequential read (write) /
/// erase blocks.
class FlashMemory {
public:
	virtual ~FlashMemory() = default;
	inline FlashMemory(const FlashMemoryGeometry &aFlashMemoryGeometry):
		flashMemoryGeometry{aFlashMemoryGeometry}
	{
	}

	/// \brief writeBlock
	/// \param aWriteBlockId Id of a writable block
	/// \param aWriteBlockOffset Offset within the block, must not exceed
	/// `getFlashMemoryGeometry().writeBlockSize`.
	///
	/// - The implementor MUST ensure that after write operation, the block
	/// stores the provided data in the specified range;
	/// - The implementor MUST preserve the remaning data in the memory chip's
	/// write block, **if** the device provides block caching;
	/// - The implementor MUST NOT perform dynamic memory allocation;
	/// - The implementor SHOULD NOT use additional buffers;
	virtual Error writeBlock(std::uint32_t aWriteBlockId, std::uint32_t aWriteBlockOffset, const std::uint8_t *aData,
		std::size_t aDataLength) = 0;

	/// \brief readBlock
	/// \param aReadBlockId Id of a readable (writable) block
	/// \param aReadBlockOffset The offset within the specified block
	virtual Error readBlock(std::uint32_t aReadBlockId, std::uint32_t aReadBlockOffset, std::uint8_t *aBuffer,
		std::size_t aBufferSize) = 0;

	/// \details "Erase block" is not to be confused with "write block"
	virtual Error eraseBlock(std::uint32_t aEraseBlockId) = 0;

	inline const FlashMemoryGeometry &getFlashMemoryGeometry() const
	{
		return flashMemoryGeometry;
	}

private:
	FlashMemoryGeometry flashMemoryGeometry;
};

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_DEVICE_FLASHMEMORY_HPP
