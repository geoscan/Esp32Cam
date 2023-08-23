//
// FlashMemory.hpp
//
// Created on: Aug 18, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORY_HPP
#define COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORY_HPP

#include "system/Error.hpp"
#include <cstdint>

namespace Sys {

/// Describes the structure of a flash memory device. Inspired by nuttx's
/// `mtd_geometry_s`.
/// \details The actual device may provide multiple levels of addressing: pane,
/// block, page, word, byte, bit. It's up to the implementor to provide
/// necessary conversions.
struct FlashMemoryGeometry {
	/// The atomic readable / writable chunk of data, in bytes
	std::uint32_t writeBlockSize;

	/// The size of an atomic eraseable chunk, in bytes. Multiple of `writeBlockSize`
	std::uint32_t eraseBlockSize;

	/// Number of erase blocks
	std::uint32_t nEraseBlocks;

	/// Counts the number of writeable blocks
	inline std::uint32_t countWriteBlocks() const
	{
		return eraseBlockSize / writeBlockSize * nEraseBlocks;
	}

	inline bool checkWriteLengthIsMultiple(std::uint32_t aWriteLength) const
	{
		return aWriteLength > 0 && aWriteLength % writeBlockSize == 0;
	}

	inline bool checkEraseLengthIsMultiple(std::uint32_t aEraseLength) const
	{
		return aEraseLength > 0 && aEraseLength % eraseBlockSize == 0;
	}

	inline std::uint32_t convertWriteBlockOffsetIntoAddress(std::uint32_t aWriteBlockOffset) const
	{
		return aWriteBlockOffset * writeBlockSize;
	}

	inline std::uint32_t convertEraseBlockOffsetIntoAddress(std::uint32_t aEraseBlockOffset) const
	{
		return aEraseBlockOffset * eraseBlockSize;
	}
};

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
	/// \param aBlockOffset Offset for a writable block. Byte offset, if
	/// bytewise write operations are available for a device
	/// \param aData
	/// \param aDataLength Must be multiple of
	/// `FlashMemoryGeometry::writeBlockSize`
	virtual Error writeBlocks(std::uint32_t aWriteBlockOffset, const std::uint8_t *aData, std::size_t aDataLength) = 0;

	/// \brief readBlock
	/// \param aReadBlockOffset The offset for an atomically read block
	/// \param anReadBlocks The number of block to read
	/// \param aBuffer
	/// \param aBufferSize Must be a multiple of `FlashMemoryGeometry::writeBlockSize`
	virtual Error readBlocks(std::uint32_t aReadBlockOffset, std::uint32_t anReadBlocks, std::uint8_t *aBuffer,
		std::size_t aBufferSize) = 0;

	virtual Error eraseBlocks(std::uint32_t aEraseBlockOffset, std::uint32_t anBlocks) = 0;

	inline const FlashMemoryGeometry getFlashMemoryGeometry() const
	{
		return flashMemoryGeometry;
	}

private:
	FlashMemoryGeometry flashMemoryGeometry;
};

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_DEVICE_FLASHMEMORY_HPP
