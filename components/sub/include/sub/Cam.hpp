//
// Cam.hpp
//
// Created on: Apr 21, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef SUB_INCLUDE_SUB_CAM_HPP_
#define SUB_INCLUDE_SUB_CAM_HPP_

#include "sub/Subscription.hpp"

namespace Sub {
namespace Cam {
namespace Topic {

struct Shot;

}  // namespace Topic

using Shot = Sub::NoLockKey<bool(const char *), Topic::Shot>;

}  // namespace Rout
}  // namespace Cam

#endif // SUB_INCLUDE_SUB_CAM_HPP_
