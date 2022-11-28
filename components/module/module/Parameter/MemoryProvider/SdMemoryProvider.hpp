//
// SdMemoryProvider.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(MODULE_MODULE_PARAMETER_MEMORYPROVIDER_HPP_)
#define MODULE_MODULE_PARAMETER_MEMORYPROVIDER_HPP_

#include "module/Parameter/MemoryProvider.hpp"
#include <memory>
#include <atomic>

class cJSON;

namespace Mod {
namespace Par {

/// \brief Wrapper over FAT / SPI SD card. Loads / stores parameter from json
/// using standard C file library. Performs (de)initialization of SD storage.
class SdMemoryProvider : public MemoryProvider {
public:
	Result load(const ParameterDescription &, Variant &) override;
	Result save(const ParameterDescription &, const Variant &value) override;
private:
	/// \brief Performs filesystem and config file checks.
	/// \post If the method results in success, the following may be relied
	/// upon:
	/// 1. The SD card has been mounted and initialized;
	/// 2. The filesystem has been successfully attached;
	/// 3. The config file is already present in the filesystem (has been
	/// created as a result of the call)
	static Result configFileEnsureExists();
	/// \brief Reads the content of an entire config file into RAM buffer,
	static Result configFileRead(std::unique_ptr<std::uint8_t[]> &buffer);
	/// \brief Parses the `buffer` which supposed to store JSON. If unable to
	/// do so, opens the file, and overwrites it with a correct stub.
	static Result configFileEnsureFormat(const std::unique_ptr<std::uint8_t[]> &buffer, cJSON **cjson);
};

}  // namespace Par
}  // namespace Mod

#endif // MODULE_MODULE_PARAMETER_MEMORYPROVIDER_HPP_
