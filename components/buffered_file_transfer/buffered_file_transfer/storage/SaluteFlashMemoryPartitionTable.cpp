//
// SaluteFlashMemoryPartitionTable.cpp
//
// Created on: Sep 27, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#include "utility/al/Crc32.hpp"
#include <array>

#include "SaluteFlashMemoryPartitionTable.hpp"

namespace Bft {

/// \brief Establishes mapping between file name (through the use of its hash),
/// and file address
struct MemoryPartitionTableEntry {
	std::uint32_t fileNameHash;
	std::uint32_t flashMemoryAddress;

	constexpr MemoryPartitionTableEntry(std::uint32_t aFileNameHash, std::uint32_t aFlashMemoryAddress):
		fileNameHash{aFileNameHash},
		flashMemoryAddress{aFlashMemoryAddress}
	{
	}
};

static constexpr std::array<MemoryPartitionTableEntry, 1> ksMemoryPartitionTable {{
	{Ut::Al::Crc32Constexpr::calculateCrc32("show"), 0},  // TODO change flash memory address
}};

bool SaluteFlashMemoryPartitionTable::tryGetFlashMemoryAddressByFile(const File &aFile,
	uint32_t &aOutFlashMemoryAddress) const
{
	for (const auto &memoryPartitionTableEntry : ksMemoryPartitionTable) {
		if (aFile.getFileNameHash() == memoryPartitionTableEntry.fileNameHash) {
			aOutFlashMemoryAddress = memoryPartitionTableEntry.flashMemoryAddress;

			return true;
		}
	}

	return false;
}

}  // Bft
