//
// Time.hpp
//
// Created on: Aug 19, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_AL_TIME_HPP_)
#define UTILITY_UTILITY_AL_TIME_HPP_

#include "utility/time.hpp"

namespace Ut {
namespace Al {

// Check if required time period has already passed
bool expired(const Time sinceUs, Time periodUs);

}  // namespace Al
}  // namespace Ut

#endif // UTILITY_UTILITY_AL_TIME_HPP_
