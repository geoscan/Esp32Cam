//
// FlashMemoryGeometry.hpp
//
// Created on: Sep 21, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORYGEOMETRY_HPP
#define COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORYGEOMETRY_HPP

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

	inline std::uint32_t convertWriteBlockOffsetIntoAddress(std::uint32_t aWriteBlockOffset) const
	{
		return aWriteBlockOffset * writeBlockSize;
	}

	inline std::uint32_t convertEraseBlockOffsetIntoAddress(std::uint32_t aEraseBlockOffset) const
	{
		return aEraseBlockOffset * eraseBlockSize;
	}
};


}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_MIDDLEWARE_FLASHMEMORYGEOMETRY_HPP
