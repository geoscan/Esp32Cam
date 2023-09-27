//
// FlashMemoryPartitionTable.hpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#ifndef COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFET_FLASHMEMORYPARTITIONTABLE_HPP_
#define COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFET_FLASHMEMORYPARTITIONTABLE_HPP_

#include "buffered_file_transfer/storage/File.hpp"

namespace Bft {

/// \brief Converts file name to address in flash memory
class FlashMemoryPartitionTable {
public:
	virtual ~FlashMemoryPartitionTable() = default;
	virtual bool tryGetFlashMemoryAddressByFile(const File &aFile, std::uint32_t &aOutFlashMemoryAddress) = 0;
};

}  // Bft

#endif // COMPONENTS_BUFFERED_FILE_TRANSFER_BUFFERED_FILE_TRANSFET_FLASHMEMORYPARTITIONTABLE_HPP_
