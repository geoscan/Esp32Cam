//
// Fs.hpp
//
// Created on: Nov 25, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_SYSTEM_FS_HPP_)
#define UTILITY_UTILITY_SYSTEM_FS_HPP_

#include <stdio.h>
#include <cstdint>

namespace Ut {
namespace Fs {

/// \brief Returns file size
/// \pre The file must be opened
/// \post Current position will be restored
std::size_t fileSize(FILE *fp);

}  // namespace Fs
}  // namespace Ut

#endif // UTILITY_UTILITY_SYSTEM_FS_HPP_
