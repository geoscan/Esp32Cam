//
// SaluteFlashMemoryPartitionTable.hpp
//
// Created on: Sep 27, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_SALUTEFLASHMEMORYPARTITIONTABLE_HPP
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_SALUTEFLASHMEMORYPARTITIONTABLE_HPP

#include "buffered_file_transfer/storage/FlashMemoryPartitionTable.hpp"

namespace Bft {

/// Defines SPI flash memory layout for Salute drones
class SaluteFlashMemoryPartitionTable : public FlashMemoryPartitionTable {
public:
	bool tryGetFlashMemoryAddressByFile(const File &aFile, std::uint32_t &aOutFlashMemoryAddress) const override;
};

}  // Bft

#endif  // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFER_STORAGE_SALUTEFLASHMEMORYPARTITIONTABLE_HPP
