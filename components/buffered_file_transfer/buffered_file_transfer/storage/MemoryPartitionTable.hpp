//
// MemoryPartitionTable.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//


#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_MEMORYPARTITIONTABLE_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_MEMORYPARTITIONTABLE_HPP

#include "buffered_file_transfer/storage/File.hpp"
#include <memory>
#include <vector>

namespace Bft {

// TODO
struct MemoryPartitionTableEntry {
	static constexpr const std::size_t kEntryNameMaxLength = 16;
	std::uint32_t flashMemoryAddress;
};

/// \brief A table with a set of predefined, preallocated entries for
/// converting a file's path into an address in flash memory.
class MemoryPartitionTable {
public:
	virtual ~MemoryPartitionTable() = default;

	/// \brief performs the conversion.
	/// \returns true, on successful conversion. False otherwise
	virtual bool tryGetFlashMemoryAddressByFile(const File &aFile, std::uint32_t &aOutAddress) const = 0;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_MEMORYPARTITIONTABLE_HPP
